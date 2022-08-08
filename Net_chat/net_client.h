//
// Created by airwalk on 04-Jul-22.
//

#pragma once

#include "net_common.h"
#include "net_message.h"
#include "net_tsqueue.h"
#include "net_connection.h"

namespace olc
{
    namespace net
    {
        template<class T>
        class client_interface
        {
        public:

            client_interface()
            {}
            virtual ~client_interface()
            {
                Disconnect();
            }

            bool Connect(const std::string& host, const uint16_t port)
            {
                try
                {
                    asio::ip::tcp::resolver resolver(m_context);
                    asio::ip::tcp::resolver::results_type m_endpoints = resolver.resolve(host, std::to_string(port));

                    m_connection = std::make_unique<connection<T>>(connection<T>::owner::client,
                            m_context, asio::ip::tcp::socket(m_context), m_qMessagesIn);

                    m_connection->ConnectToServer(m_endpoints);


                    thrContext = std::thread([this](){m_context.run();});
                }
                catch (std::exception& ex)
                {
                    std::cerr << "Client exception: " << ex.what() << "\n";
                    return false;
                }
                return true;
            }

            void Disconnect()
            {
                if(IsConnected()){
                    m_connection->Disconnect();
                }
                m_context.stop();
                if(thrContext.joinable())
                    thrContext.join();

                m_connection.release();
            }
            bool IsConnected()
            {
                if(m_connection)
                   return m_connection->IsConnected();
                else
                    return false;
            }

            void Send(const message<T>& msg){
                if (IsConnected()) {
                    m_connection->Send(msg);
                }
            }

            tsqueue<owned_message<T>>& Incoming(){
                return m_qMessagesIn;
            }

            uint32_t GetConnectedToId() const {
                return ConnectedToId;
            }

            uint32_t ChangeConnectedToId(uint32_t newid)  {
                ConnectedToId = newid;
                return ConnectedToId;
            }



        private:
            tsqueue<owned_message<T>> m_qMessagesIn;



        protected:
            asio::io_context m_context;
            std::thread thrContext;
            std::unique_ptr<connection<T>> m_connection;
            uint32_t ConnectedToId = 0;
            uint32_t ConnectionRequestId = 0;
            std::mutex muxConnect;
            std::condition_variable cvConnect;
        };

    }

}
