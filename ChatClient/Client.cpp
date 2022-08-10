#include "Client.h"

Custom_Client::~Custom_Client()
{
*this << Command{CommandPrefix::CloseClient};
Disconnect();
}

void Custom_Client::PingServer()
{
  olc::net::message<CustomMsgTypes> msg;
  msg.header.id = CustomMsgTypes::ServerPing;
  std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
  msg << timeNow;
  Send(msg);
}

void Custom_Client::MessageAll()
{
  olc::net::message<CustomMsgTypes> msg;
  msg.header.id = CustomMsgTypes::MessageAll;
  Send(msg);
}

void Custom_Client::GetClientList()
{
  olc::net::message<CustomMsgTypes> msg;
  msg.header.id = CustomMsgTypes::ClientList;
  Send(msg);
}

void Custom_Client::ConnectRequest(uint32_t id)
{
  olc::net::message<CustomMsgTypes> msg;
  msg.header.id = CustomMsgTypes::RequestToClient;
  msg << id;
  Send(msg);
}

void Custom_Client::SendToClient(std::string text)
{
  olc::net::message<CustomMsgTypes> msg;
  msg.header.id = CustomMsgTypes::TextClient;
  msg << text;
  Send(msg);
}

void Custom_Client::OpenChatWithClient()
{
  std::cout << "Connected with " << ConnectedToId << ". Type -close to exit\n>>";
  std::string DataToClient;
  DataToClient = "";
  std::getline(std::cin, DataToClient);
  while(GetConnectedToId() != 0)
  {
    if (DataToClient == "-close")
    {
      *this << Command{CommandPrefix::CloseClient};
    } else
    {
      SendToClient(DataToClient);
      std::cout << ">>";
    }
    DataToClient = "";
    std::getline(std::cin, DataToClient);
  }
  std::cout << "\n>";
}

void Custom_Client::ExecuteMessages()
{
  while (1)
  {
    if (IsConnected() && !Incoming().empty())
    {
      auto msg = Incoming().pop_front().msg;
      std::cout << "\n";
      switch (msg.header.id)
      {
        case CustomMsgTypes::ServerPing:
        {
          std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
          std::chrono::system_clock::time_point timeThen;
          msg >> timeThen;
          std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n>";
        } break;

        case CustomMsgTypes::ServerMessage:
        {
          uint32_t clientID;
          msg >> clientID;
          std::cout << "Hello from [" << clientID << "]\n>";
        } break;

        case CustomMsgTypes::ServerAccept:
        {
          std::cout << "Server Accepted Connection\n";
          uint32_t ID;
          msg >> ID;
          std::cout << "Your ID is " << ID << "\n>";
        } break;

        case CustomMsgTypes::ClientList:
        {
          std::cout << "Client List:\n";
          uint32_t id;
          auto listSize = static_cast<size_t>(msg.size()/sizeof(uint32_t));
          for(size_t i = listSize; i > 0; i--)
          {
            msg >> id;
            std::cout << "Client id #:" << i << " is " << id << "\n";
          }
          std::cout << ">";
        } break;

        case CustomMsgTypes::RequestToClient:
        {
          uint32_t id;
          msg >> id;
          switch (id)
          {
            case 0:
            {
              std::unique_lock<std::mutex> ul(muxConnect);
              ConnectionRequestId = 0;
              cvConnect.notify_one();
              std::cout << "No such client exist\n>";
            } break;

            case 1:
            {
              std::unique_lock<std::mutex> ul(muxConnect);
              ConnectionRequestId = 0;
              cvConnect.notify_one();
              std::cout << "client busy\n>";
            } break;

            default:
            {
              std::cout << "Request to chat from " << id << ". Type Accept or Decline to answer\n>";
              ConnectionRequestId = id;
            } break;
          }
        } break;

        case CustomMsgTypes::TextClient:
        {
          std::cout << GetConnectedToId() << " says: ";
          std::string text;
          msg >> text;
          std::cout << text << "\n>>";
        } break;

        case CustomMsgTypes::RequestAnswer:
        {
          std::cout << "Request Answer\n";
          int code;
          msg >> code;
          if(code)
          {
            ChangeConnectedToId(ConnectionRequestId);
            std::unique_lock<std::mutex> ul(muxConnect);
            ConnectionRequestId = 0;
            cvConnect.notify_one();
            std::cout << "Client accepted\n";
            break;
          } else
          {
            std::unique_lock<std::mutex> ul(muxConnect);
            ConnectionRequestId = 0;
            cvConnect.notify_one();
            std::cout << "Declined\n>";
          }
        } break;

        case CustomMsgTypes::CloseClient:
        {
          ChangeConnectedToId(0);
          ConnectionRequestId = 0;
          std::cout << "Connection with client is closed. Press enter to continue\n";
        } break;



        default:
        {
          std::cout << "Unknown message\n>";
        } break;
      }
    } //switch (msg.header.id)
  } //while(1)
} // void ExecuteMessages()