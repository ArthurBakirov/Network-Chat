#ifndef CLIENT_H
#define CLIENT_H
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

  friend Custom_Client& operator<<(Custom_Client& oc, Command c);

  void ExecuteMessages();
};

Custom_Client& operator<<(Custom_Client& oc, Command c);

#endif /* End of CLIENT_H */