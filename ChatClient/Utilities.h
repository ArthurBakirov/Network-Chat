#pragma once

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

enum class CommandPrefix : uint32_t
{
  ServerPing,
  ClientList,
  HelloAll,
  ConnectTo,
  RequestAccept,
  RequestDecline,
  CloseClient,
  Help,
  Blank,
};

struct Command
{
  CommandPrefix prefix = CommandPrefix::Blank;
  uint32_t id = 0;
};

inline Command StrToCommand(const std::string& string)
{
  static std::unordered_map<std::string,CommandPrefix> const SimpleCommands =
        {
              {"Ping",CommandPrefix::ServerPing},
              {"Clients",CommandPrefix::ClientList},
              {"HelloAll",CommandPrefix::HelloAll},
              {"Accept",CommandPrefix::RequestAccept},
              {"Decline",CommandPrefix::RequestDecline},
              {"Help",CommandPrefix::Help}
        };
  static std::unordered_map<std::string,CommandPrefix> const CommandsWithID =
        {
              {"ConnectTo",CommandPrefix::ConnectTo}
        };
  std::string::size_type pos = string.find_first_of(" ");
  std::string prefix = string.substr(0, pos); // the first part before the space
  auto it = SimpleCommands.find(prefix);
  if (it != SimpleCommands.end())
  {
    return {it->second};
  } else if(it = CommandsWithID.find(prefix); it != CommandsWithID.end())
  {
    try
    {
      uint32_t id = static_cast<uint32_t>(std::stoul(string.substr(pos + 1)));  // the part after the space
      return {it->second, id};
    } catch (...)
    {
      std::cout << "Invalid ID" << "\n";
    }
  }
  return {CommandPrefix::Blank};
} //Command StrToCommand(const std::string& string)
