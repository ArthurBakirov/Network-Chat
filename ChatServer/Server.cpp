#include "Server.h"

bool CustomServer::OnClientConnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client)
{
  olc::net::message<CustomMsgTypes> msg;
  msg.header.id = CustomMsgTypes::ServerAccept;
  uint32_t ID = nIDCounter;
  msg << ID;
  client->Send(msg);
  return true;
}

void CustomServer::OnClientDisconnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client)
{
  std::cout << "Removing client [" << client->GetID() << "]\n";
}

void CustomServer::OnMessage(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client, olc::net::message<CustomMsgTypes>& msg)
{
  switch (msg.header.id)
  {
    case CustomMsgTypes::ServerAccept:
    {
      std::cout << "Server Accepted Connection\n";
    } break;

    case CustomMsgTypes::ServerPing:
    {
      std::cout << "[" << client->GetID() << "]: Server Ping\n";
      client->Send(msg);
    } break;

    case CustomMsgTypes::MessageAll:
    {
      std::cout << "[" << client->GetID() << "]: Message All\n";
      olc::net::message<CustomMsgTypes> outMsg;
      outMsg.header.id = CustomMsgTypes::ServerMessage;
      outMsg << client->GetID();
      MessageAllClient(outMsg, client);
    } break;

    case CustomMsgTypes::ClientList:
    {
      std::cout << "[" << client->GetID() << "]: Client List\n";
      auto List = GetClientList();
      olc::net::message<CustomMsgTypes> outMsg;
      outMsg.header.id = CustomMsgTypes::ClientList;
      for (uint32_t id : List)
      {
        outMsg << id;
      }
      client->Send(outMsg);
    } break;

    case CustomMsgTypes::RequestToClient:
    {
      uint32_t ID;
      msg >> ID;
      std::cout << "[" << client->GetID() << "]: Connection request to " << ID << "\n";
      olc::net::message<CustomMsgTypes> ResponseMsg;
      ResponseMsg.header.id = CustomMsgTypes::RequestToClient;
      bool ClientFound = false;
      for (auto client_ : m_dequeConnections)
      {
        if (client_->GetID() == ID)
        {
          ClientFound = true;
          if (ConnectClients(client->GetID(), ID))
          {
            ResponseMsg << ID;
            olc::net::message<CustomMsgTypes> RequestMsg;
            RequestMsg.header.id = CustomMsgTypes::RequestToClient;
            RequestMsg << client->GetID();
            client_->Send(RequestMsg);
          } else
          {
            //Clients is busy code 1
            ResponseMsg << uint32_t(1);
            client->Send(ResponseMsg);
          }
          break; //for cycle
        }
      } //end of for cycle
      if (!ClientFound)
      {
        //Clients is not found code 0
        ResponseMsg << uint32_t(0);
        client->Send(ResponseMsg);
      }
    } break; //Case request for client


    case CustomMsgTypes::TextClient:
    {
      std::cout << "[" << client->GetID() << "]: Text Client\n";
      uint32_t ID = ClientConnections.at(client->GetID());
      for (auto client_ : m_dequeConnections)
      {
        if (client_->GetID() == ID)
        {
          if(ClientConnections.count(client->GetID()) > 0 && client_->IsConnected())
          {
            client_->Send(msg);
            break; //for cycle
          } else
          {
            DisconnectClients(ID, client->GetID());
            olc::net::message<CustomMsgTypes> outMsg;
            outMsg.header.id = CustomMsgTypes::CloseClient;
            client->Send(outMsg);
          }
        }
      }
    } break;

    case CustomMsgTypes::RequestAnswer:
    {
      std::cout << "[" << client->GetID() << "]: Request Answer\n";
      uint32_t ID = ClientConnections.at(client->GetID());
      for (auto client_ : m_dequeConnections)
      {
        if (client_->GetID() == ID)
        {
          client_->Send(msg);
          break; //for cycle
        }
      }
    } break;

    case CustomMsgTypes::CloseClient:
    {
      std::cout << "[" << client->GetID() << "]: Close client\n";
      olc::net::message<CustomMsgTypes> outMsg;
      outMsg.header.id = CustomMsgTypes::CloseClient;
      uint32_t ID = ClientConnections.at(client->GetID());
      for (auto client_ : m_dequeConnections)
      {
        if (client_->GetID() == ID)
        {
          client_->Send(outMsg);
          break;
        }
      }
      DisconnectClients(client->GetID(), ID);
    } break;

  } //switch
} //onmessage function

