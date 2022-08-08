#include <iostream>

#include <olc_net.h>
#include <future>


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


struct Command{
    CommandPrefix prefix = CommandPrefix::Blank;
    uint32_t id = 0;
};

class Custom_Client : public olc::net::client_interface<CustomMsgTypes>{
public:

     ~Custom_Client() override {
        *this << Command{CommandPrefix::CloseClient};
        Disconnect();
    }

    void PingServer(){
        olc::net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::ServerPing;
        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
        msg << timeNow;
        Send(msg);
    }
    void MessageAll() {
        olc::net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::MessageAll;
        Send(msg);
    }

    void GetClientList(){
        olc::net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::ClientList;
        Send(msg);
    }

    void ConnectRequest(uint32_t id) {
        olc::net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::RequestToClient;
        msg << id;
        Send(msg);
    }

    void SendToClient(std::string text){
        olc::net::message<CustomMsgTypes> msg;
        msg.header.id = CustomMsgTypes::TextClient;
        msg << text;
        Send(msg);
    }

    void OpenChatWithClient(){

        std::cout << "Connected with " << ConnectedToId << ". Type -close to exit\n>>";
        std::string DataToClient;

        DataToClient = "";
        std::getline(std::cin, DataToClient);
        while(GetConnectedToId() != 0){
            if (DataToClient == "-close"){
                *this << Command{CommandPrefix::CloseClient};
            } else {
                SendToClient(DataToClient);
                std::cout << ">>";
            }

            DataToClient = "";
            std::getline(std::cin, DataToClient);

        }
        std::cout << "\n>";

    }

    friend Custom_Client& operator<<(Custom_Client& oc, Command c)
    {
        switch(c.prefix)
        {
            case CommandPrefix::ServerPing   : oc.PingServer();    break;
            case CommandPrefix::ClientList: oc.GetClientList(); break;
            case CommandPrefix::HelloAll  : {
                oc.MessageAll();
                std::cout<< "Hello sent to every client\n>";
            } break;

            case CommandPrefix::ConnectTo  : {
                std::cout << "Connecting to " << c.id << ". Press ESC key to abort request\n";
                oc.ConnectRequest(c.id);

                oc.ConnectionRequestId = c.id;

                std::unique_lock<std::mutex> ul(oc.muxConnect);

                std::future<void> abort = std::async([&](){
                    bool key, old_key = false;
                    while (oc.ConnectionRequestId == c.id) {
                        if (GetForegroundWindow() == GetConsoleWindow())
                            key = GetAsyncKeyState(27) & 0x8000;
                        if (key && !old_key){
                            oc << Command{CommandPrefix::CloseClient};
                            std::unique_lock<std::mutex> ul(oc.muxConnect);
                            oc.cvConnect.notify_one();
                            std::cin.get();
                            std::cout << ">";
                        }
                        old_key = key;
                        }

                });

                while (oc.ConnectionRequestId == c.id){
                    oc.cvConnect.wait(ul);
                }

                if (oc.GetConnectedToId() >= 10000) {
                    oc.OpenChatWithClient();
                }


            } break;


            case CommandPrefix::RequestAccept : {

                if(oc.ConnectionRequestId >= 10000) {
                    olc::net::message<CustomMsgTypes> msg;
                    msg.header.id = CustomMsgTypes::RequestAnswer;
                    msg << int(1);
                    oc.Send(msg);
                    std::cout << "Accept sent" << "\n";
                    oc.ChangeConnectedToId(oc.ConnectionRequestId);

                    oc.OpenChatWithClient();
                } else {
                    std::cout << "Nothing to accept\n>";
                }

            } break;

            case CommandPrefix::RequestDecline : {
                if(oc.ConnectionRequestId >= 10000) {
                    olc::net::message<CustomMsgTypes> msg;
                    msg.header.id = CustomMsgTypes::RequestAnswer;
                    msg << int(0);
                    oc.Send(msg);
                    std::cout << "Decline sent" << "\n";
                } else {
                    std::cout << "Nothing to decline\n>";
                }
            } break;

            case CommandPrefix::CloseClient : {
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

            default    : std::cout << "Unknown command\n>";
        }

        return oc;
    }


    void ExecuteMessages(){


        while (1) {

            if (IsConnected()){

                if (!Incoming().empty()){

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


                        }
                            break;

                        case CustomMsgTypes::ServerMessage:
                        {
                            uint32_t clientID;
                            msg >> clientID;
                            std::cout << "Hello from [" << clientID << "]\n>";
                        }
                            break;

                        case CustomMsgTypes::ServerAccept:
                        {
                            std::cout << "Server Accepted Connection\n";
                            uint32_t ID;
                            msg >> ID;
                            std::cout << "Your ID is " << ID << "\n>";
                        }
                            break;



                        case CustomMsgTypes::ClientList:
                        {
                            std::cout << "Client List:\n";
                            uint32_t id;
                            auto listSize = static_cast<size_t>(msg.size()/sizeof(uint32_t));
                            for(size_t i = listSize; i > 0; i--){
                                msg >> id;
                                std::cout << "Client id #:" << i << " is " << id << "\n";}
                        }
                        std::cout << ">";
                            break;


                        case CustomMsgTypes::RequestToClient:
                        {
                            uint32_t id;
                            msg >> id;
                            switch (id) {
                                case 0: {
                                    std::unique_lock<std::mutex> ul(muxConnect);
                                    ConnectionRequestId = 0;
                                    cvConnect.notify_one();
                                    std::cout << "No such client exist\n>";
                                } break;
                                case 1: {
                                    std::unique_lock<std::mutex> ul(muxConnect);
                                    ConnectionRequestId = 0;
                                    cvConnect.notify_one();
                                    std::cout << "client busy\n>";
                                } break;
                                default: {

                                    std::cout << "Request to chat from " << id << ". Type Accept or Decline to answer\n>";
                                    ConnectionRequestId = id;


                                } break;
                            }
                        }
                            break;

                        case CustomMsgTypes::TextClient:
                        {
                            std::cout << GetConnectedToId() << " says: ";
                            std::string text;
                            msg >> text;
                            std::cout << text << "\n>>";
                        }
                            break;

                        case CustomMsgTypes::RequestAnswer:{
                            std::cout << "Request Answer\n";
                            int code;
                            msg >> code;
                            if(code){
                                ChangeConnectedToId(ConnectionRequestId);
                                std::unique_lock<std::mutex> ul(muxConnect);
                                ConnectionRequestId = 0;
                                cvConnect.notify_one();
                                std::cout << "Client accepted\n";
                                break;
                            } else {
                                std::unique_lock<std::mutex> ul(muxConnect);
                                ConnectionRequestId = 0;
                                cvConnect.notify_one();
                                std::cout << "Declined\n>";
                            }
                        }
                            break;

                        case CustomMsgTypes::CloseClient:
                        {
                            ChangeConnectedToId(0);
                            ConnectionRequestId = 0;
                            std::cout << "Connection with client is closed. Press enter to continue\n";
                        } break;



                        default:{
                            std::cout << "Unknown message\n>";
                        } break;




                    }


                }

            }
        }

    }



};





Command StrToCommand(const std::string& string){

    static std::unordered_map<std::string,CommandPrefix> const SimpleCommands = {
            {"Ping",CommandPrefix::ServerPing},
            {"Clients",CommandPrefix::ClientList},
            {"HelloAll",CommandPrefix::HelloAll},
            {"Accept",CommandPrefix::RequestAccept},
            {"Decline",CommandPrefix::RequestDecline},
            {"Help",CommandPrefix::Help}
    };

    static std::unordered_map<std::string,CommandPrefix> const CommandsWithID = {
            {"ConnectTo",CommandPrefix::ConnectTo}
    };


    std::string::size_type pos = string.find_first_of(" ");
    std::string prefix = string.substr(0, pos); // the first part before the space



    auto it = SimpleCommands.find(prefix);

    if (it != SimpleCommands.end()) {
        return {it->second};
    } else if(it = CommandsWithID.find(prefix); it != CommandsWithID.end()) {
        try {
            uint32_t id = static_cast<uint32_t>(std::stoul(string.substr(pos + 1)));  // the part after the space
            return {it->second, id};
        } catch (...) {
            std::cout << "Invalid ID" << "\n";
        }
    }
        return {CommandPrefix::Blank};

}




int main() {
Custom_Client c;
c.Connect("127.0.0.1", 60000);

if (c.IsConnected()) {
    std::cout << "Connected to the host. Type Help for info";
}

std::string inc_data;

std::future<void> futureString = std::async(std::launch::async, &Custom_Client::ExecuteMessages, std::ref(c)) ;



while(1)
{

    inc_data = "";

std::getline(std::cin, inc_data);

    if (inc_data != "")
    c << StrToCommand(inc_data);
    else
        std::cout << ">";

}


    return 0;
}
