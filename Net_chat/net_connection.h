#ifndef NET_CONNECTION_H
#define NET_CONNECTION_H

#include "net_common.h"
#include "net_message.h"
#include "net_tsqueue.h"

namespace olc
{
  namespace net
  {
    template<class T>
    class server_interface;

    template<class T>
    class connection : public std::enable_shared_from_this<connection<T>>
    {
    public:
      enum class owner
      {
        server,
        client
      };

      connection(owner parent, asio::io_context& context, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& queue)
            : m_asioContext(context), m_socket(std::move(socket)), m_qMessagesIn(queue)
      {
        m_nOwnerType = parent;
        if(parent == owner::server)
        {
          Validaton_code_out = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
          Validaton_code_sum = Scramble(Validaton_code_out);
        }
      }

      virtual ~connection()
      {}

      uint32_t GetID() const
      {
        return id;
      }

      void ConnectToClient(olc::net::server_interface<T>* server, uint32_t uid = 0)
      {
        if (m_nOwnerType == owner::server)
        {
          if(m_socket.is_open())
          {
            id = uid;
            WriteValidation();
            ReadValidation(server);
          }
        }
      }

      void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
      {
        if (m_nOwnerType == owner::client)
        {
          asio::async_connect
                (m_socket, endpoints, [this](std::error_code ec,asio::ip::tcp::endpoint endpoint)
                {
                  if (!ec)
                  {
                    ReadValidation();
                  }
                });
        }
      }

      void Disconnect()
      {
        if (IsConnected())
        {
          asio::post
                (m_asioContext,[this]()
                {
                  m_socket.close();
                });
        }
      }

      bool IsConnected() const
      {
        return m_socket.is_open();
      }

      void Send(const message<T>& msg)
      {
        asio::post
              (m_asioContext, [this, msg]()
              {
                bool bWritingMessage = !m_qMessagesOut.empty();
                m_qMessagesOut.push_back(msg);
                if(!bWritingMessage)
                {
                  WriteHeader();
                }
              });
      }

    private:
      void ReadHeader()
      {
        asio::async_read
              (m_socket, asio::buffer(&m_msgBuff.header, sizeof(message_header<T>)),
                    [this](std::error_code error, std::size_t bytes)
               {
                 if(!error)
                 {
                   if(m_msgBuff.header.size > 0)
                   {
                     m_msgBuff.body.resize(m_msgBuff.header.size);
                     ReadBody();
                   } else
                   {
                     AddToIncomimgMessageQueue();
                   }
                 } else
                 {
                   std::cout <<"["<< id <<  "] Header read fail id: " << id << std::endl;
                   m_socket.close();
                 }
               });
      }

      void ReadBody()
      {
        asio::async_read
              (m_socket, asio::buffer(m_msgBuff.body.data(), m_msgBuff.body.size()),
                    [this](std::error_code error, std::size_t bytes)
               {
                 if(!error)
                 {
                   AddToIncomimgMessageQueue();
                 } else
                 {
                   std::cout <<"["<< id <<  "] Body read fail id: " << id << std::endl;
                   m_socket.close();
                 }
               });
      }

      void WriteHeader()
      {
        asio::async_write
              (m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)),
                    [this](const std::error_code& error, std::size_t bytes)
               {
                 if(!error)
                 {
                   if(m_qMessagesOut.front().body.size() > 0)
                   {
                     WriteBody();
                   } else
                   {
                     m_qMessagesOut.pop_front();
                     if(!m_qMessagesOut.empty())
                     {
                       WriteHeader();
                     }
                   }
                 } else
                 {
                   std::cout <<"["<< id <<  "] Head write fail"  << std::endl;
                   m_socket.close();
                 }
               });
      }


      void WriteBody()
      {
        asio::async_write
              (m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
                    [this](const std::error_code& error, std::size_t bytes)
               {
                 if(!error)
                 {
                   auto msg = m_qMessagesOut.pop_front();
                   if(!m_qMessagesOut.empty())
                   {
                     WriteHeader();
                   }
                 } else
                 {
                   std::cout <<"["<< id <<  "] Body write fail" << std::endl;
                   m_socket.close();
                 }
               });

      }

      void AddToIncomimgMessageQueue()
      {
        if (m_nOwnerType == owner::server)
        {
          m_qMessagesIn.push_back({this->shared_from_this(), m_msgBuff});
        } else
        {
          m_qMessagesIn.push_back({nullptr, m_msgBuff});
        }
        ReadHeader();
      }

      uint64_t Scramble(uint64_t input_val)
      {
        uint64_t output = input_val ^ 0xdfca;
        return output;
      }

      void WriteValidation()
      {
        asio::async_write
              (m_socket, asio::buffer(&Validaton_code_out, sizeof(uint64_t)),
                    [this](const std::error_code& error, std::size_t bytes)
               {
                 if(!error)
                 {
                   if (m_nOwnerType == owner::client)
                   {
                     ReadHeader();
                   }
                 } else
                 {
                   m_socket.close();
                 }
               });
      }

      void ReadValidation(olc::net::server_interface<T>* server = nullptr)
      {
        m_socket.async_read_some
              (asio::buffer(&Validaton_code_in, sizeof(uint64_t)),
                    [this, server](std::error_code error, std::size_t bytes)
               {
                 if(!error)
                 {
                   if (m_nOwnerType == owner::server)
                   {
                     if (Validaton_code_in == Validaton_code_sum)
                     {
                       std::cout <<"Client Validation success" << std::endl;
                       ReadHeader();
                     } else
                     {
                       std::cout <<"Client disconnect. Wrong validation code" << std::endl;
                       m_socket.close();
                       server->OnClientValidated(this->shared_from_this());
                     }
                   } else
                   {
                     Validaton_code_out = Scramble(Validaton_code_in);
                     WriteValidation();
                   }

                 } else
                 {
                   std::cout <<"Client disconnect. Validation read fail" << std::endl;
                   m_socket.close();
                 }
               });

      }


    protected:
      asio::ip::tcp::socket m_socket;
      asio::io_context& m_asioContext;
      message<T> m_msgBuff;
      tsqueue<message<T>> m_qMessagesOut;
      tsqueue<owned_message<T>>& m_qMessagesIn;
      owner m_nOwnerType = owner::server;
      uint32_t id = 0;
      uint64_t Validaton_code_out = 0;
      uint64_t Validaton_code_in = 0;
      uint64_t Validaton_code_sum = 0;
    }; //connection
  } //namespace net
} //namespace olc

#endif /* End of NET_CONNECTION_H */