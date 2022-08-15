#ifndef UTILITIES_H
#define UTILITIES_H

enum class CustomMsgTypes : uint32_t
{
  ServerAccept,
  ServerDeny,
  ServerPing,
  MessageAll,
  ServerMessage,
  ClientList,
  RequestToClient,
  TextClient,
  RequestAnswer,
  CloseClient,
};

#endif /* End of UTILITIES_H */