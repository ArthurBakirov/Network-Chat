#pragma once
#include <olc_net.h>
#include "Utilities.h"

class Custom_Client : public olc::net::client_interface<CustomMsgTypes>
{
public:
  ~Custom_Client() override;

  void PingServer();

  void MessageAll();

  void GetClientList();

  void ConnectRequest(uint32_t id);

  void SendToClient(std::string text);

  void OpenChatWithClient();

  friend Custom_Client& operator<<(Custom_Client& oc, Command c)
  {
    switch(c.prefix)
    {
      case CommandPrefix::ServerPing   : oc.PingServer();    break;
      case CommandPrefix::ClientList: oc.GetClientList(); break;
      case CommandPrefix::HelloAll  :
      {
        oc.MessageAll();
        std::cout<< "Hello sent to every client\n>";
      } break;

      case CommandPrefix::ConnectTo :
      {
        std::cout << "Connecting to " << c.id << ". Press ESC key to abort request\n";
        oc.ConnectRequest(c.id);
        oc.ConnectionRequestId = c.id;
        std::unique_lock<std::mutex> ul(oc.muxConnect);
        std::future<void> abort = std::async
              ([&](){
                bool key, old_key = false;
                while (oc.ConnectionRequestId == c.id)
                {
                  if (GetForegroundWindow() == GetConsoleWindow())
                    key = GetAsyncKeyState(27) & 0x8000;
                  if (key && !old_key)
                  {
                    oc << Command{CommandPrefix::CloseClient};
                    std::unique_lock<std::mutex> ul(oc.muxConnect);
                    oc.cvConnect.notify_one();
                    std::cin.get();
                    std::cout << ">";
                  }
                  old_key = key;
                }
              });

        while (oc.ConnectionRequestId == c.id)
        {
          oc.cvConnect.wait(ul);
        }

        if (oc.GetConnectedToId() >= 10000)
        {
          oc.OpenChatWithClient();
        }
      } break; //case CommandPrefix::ConnectTo


      case CommandPrefix::RequestAccept :
      {
        if(oc.ConnectionRequestId >= 10000)
        {
          olc::net::message<CustomMsgTypes> msg;
          msg.header.id = CustomMsgTypes::RequestAnswer;
          msg << int(1);
          oc.Send(msg);
          std::cout << "Accept sent" << "\n";
          oc.ChangeConnectedToId(oc.ConnectionRequestId);
          oc.OpenChatWithClient();
        } else
        {
          std::cout << "Nothing to accept\n>";
        }
      } break;

      case CommandPrefix::RequestDecline :
      {
        if(oc.ConnectionRequestId >= 10000)
        {
          olc::net::message<CustomMsgTypes> msg;
          msg.header.id = CustomMsgTypes::RequestAnswer;
          msg << int(0);
          oc.Send(msg);
          std::cout << "Decline sent" << "\n";
        } else
        {
          std::cout << "Nothing to decline\n>";
        }
      } break;

      case CommandPrefix::CloseClient :
      {
        olc::net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::CloseClient;
        oc.Send(msg);
        oc.ChangeConnectedToId(0);
        oc.ConnectionRequestId = 0;
        std::cout << "Client closed. Press enter to continue" << "\n";
      } break;

      case CommandPrefix::Help:
      {
        std::cout << "List of commands:\n"
                     "Ping - ping server, round trip time in ms\n"
                     "Clients - get list of clients connected to the host\n"
                     "HelloAll - send hello message to all clients connected to the host\n"
                     "Accept - accept incoming request if available\n"
                     "Decline - delcilne incoming request if available\n"
                     "Help  - help information\n"
                     "ConnectTo 'client ID' - Request connection to the client with the specific ID\n>";
      } break;

      default: std::cout << "Unknown command\n>";
    }
    return oc;
  }

  void ExecuteMessages();
};
