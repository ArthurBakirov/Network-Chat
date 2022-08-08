# Network-Chat

Centralized console server and client for message exchange. Server receives messages on address 127.0.0.1 port 60000. Clients can connect to each other to exchange messages via console application. 
Based on library boost::asio, and OneLoneCoder's network framework https://github.com/OneLoneCoder/olcPixelGameEngine/tree/master/Videos/Networking/Parts1%262

List of commands:
Ping - ping server, round trip time in ms. 
Clients - get list of clients connected to the host. 
Accept - accept incoming request if available. 
Decline - delcilne incoming request if available. 
Help  - help information. 
ConnectTo 'client ID' - Request connection to the client with the specific ID. 
