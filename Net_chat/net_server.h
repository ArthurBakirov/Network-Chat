#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"

namespace olc {
  namespace net {
    template<class T>
    class server_interface
    {
    public:
      server_interface(uint16_t port) : m_asioAcceptor(m_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
      {}

      virtual ~server_interface()
      {
        Stop();
      }

      bool Start()
      {
        try
        {
          WaitForClientConnection();
          m_threadContext = std::thread([this](){m_context.run();});
        }
        catch (std::exception& e)
        {
          std::cerr <<"[SERVER] exception: " << e.what() << "\n";
          return false;
        }
        std::cout <<"[SERVER] started"<< "\n";
        return true;
      }

      void Stop()
      {
        m_context.stop();
        if(m_threadContext.joinable())
          m_threadContext.join();
        std::cout <<"[SERVER] stopped:"<< "\n";
      }

      void WaitForClientConnection()
      {
        m_asioAcceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket)
                                    {
                                      if(!ec)
                                      {
                                        std::cout <<"[SERVER] New connection: " << socket.remote_endpoint() << "\n";
                                        std::shared_ptr<connection<T>> newconn =
                                              std::make_shared<connection<T>>(connection<T>::owner::server,
                                                                              m_context, std::move(socket), m_qMessagesIn);
                                        if(OnClientConnect(newconn))
                                        {
                                          m_dequeConnections.push_back(std::move(newconn));
                                          m_dequeConnections.back()->ConnectToClient(this, nIDCounter++);
                                          std::cout << "[" << m_dequeConnections.back()->GetID() << "] Connection approved\n";
                                        } else
                                        {
                                          std::cout << "[----] Connection denied\n";
                                        }
                                      } else
                                      {
                                        std::cout <<"[SERVER] New connection error:"<< ec.message() << "\n";
                                      }
                                      WaitForClientConnection();
                                    }); //lambada function, async_accept
      }

      void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg)
      {
        if(client && client->IsConnected())
        {
          client->Send(msg);
        } else
        {
          OnClientDisconnect(client);
          client.reset();
          m_dequeConnections.erase(
                std::remove(m_dequeConnections.begin(), m_dequeConnections.begin(), client), m_dequeConnections.end());
        }
      }

      void MessageAllClient(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
      {
        bool InvalidClientExists = false;
        for (auto& client : m_dequeConnections)
        {
          if (client && client->IsConnected())
          {
            if(client != pIgnoreClient)
            {
              client->Send(msg);
            }
          } else
          {
            OnClientDisconnect(client);
            client.reset();
            InvalidClientExists = true;
          }
        }
        if (InvalidClientExists)
          m_dequeConnections.erase(
                std::remove(m_dequeConnections.begin(), m_dequeConnections.end(), nullptr), m_dequeConnections.end());
      } //MessageAllClient function

      std::vector<uint32_t> GetClientList()
      {
        bool InvalidClientExists = false;
        std::vector<uint32_t> ClientList;
        for (auto& client : m_dequeConnections)
        {
          if (client && client->IsConnected())
          {
            ClientList.push_back(client->GetID());
          } else
          {
            OnClientDisconnect(client);
            client.reset();
            InvalidClientExists = true;
          }
        }
        if (InvalidClientExists)
          m_dequeConnections.erase(
                std::remove(m_dequeConnections.begin(), m_dequeConnections.end(), nullptr), m_dequeConnections.end());
        return ClientList;
      }

      void Update(size_t nMaxMessages = -1, bool lowcpu = false)
      {
        if (lowcpu) m_qMessagesIn.wait();
        size_t MsgCount = 0;
        while(MsgCount < nMaxMessages && !m_qMessagesIn.empty())
        {
          auto msg = m_qMessagesIn.pop_front();
          OnMessage(msg.remote, msg.msg);
          MsgCount++;
        }
      }

      virtual void OnClientValidated(std::shared_ptr<connection<T>> client)
      {}

    protected:
      virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
      {
        return false;
      }

      virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
      {}

      virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg)
      {}

      bool ConnectClients(uint32_t ClientA, uint32_t ClientB)
      {
        if(ClientConnections.count(ClientA) + ClientConnections.count(ClientB) == 0 && ClientA != ClientB)
        {
          ClientConnections.emplace(ClientA, ClientB);
          ClientConnections.emplace(ClientB, ClientA);
          return true;
        }
        return false;
      }

      void DisconnectClients(uint32_t ClientA, uint32_t ClientB)
      {
        ClientConnections.erase(ClientConnections.find(ClientA));
        ClientConnections.erase(ClientConnections.find(ClientB));
      }

    protected:
      tsqueue<owned_message<T>> m_qMessagesIn;
      asio::io_context m_context;
      std::thread m_threadContext;
      asio::ip::tcp::acceptor m_asioAcceptor;
      std::deque<std::shared_ptr<connection<T>>> m_dequeConnections;
      uint32_t nIDCounter = 10000;
      std::unordered_map<uint32_t, uint32_t> ClientConnections;
    }; //class server_interface
  } //namespace net
} //namespace olc