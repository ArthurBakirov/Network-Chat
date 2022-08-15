#ifndef SERVER_H
#define SERVER_H

#include <olc_net.h>

#include "Utilities.h"

class CustomServer : public olc::net::server_interface<CustomMsgTypes>
{
public:
  CustomServer(uint16_t nPort) : olc::net::server_interface<CustomMsgTypes>(nPort)
  {}

protected:
  bool OnClientConnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client) override;

  void OnClientDisconnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client) override;

  void OnMessage(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client, olc::net::message<CustomMsgTypes>& msg) override;
};

#endif /* End of SERVER_H */