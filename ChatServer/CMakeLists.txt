cmake_minimum_required(VERSION 3.16)
project(ChatServer)

set(CMAKE_CXX_STANDARD 17)


include_directories(../Asio_lib)
include_directories(../Net_chat)

link_libraries(ws2_32 wsock32)

add_executable(ChatServer main.cpp Server.h Utilities.h Server.cpp)