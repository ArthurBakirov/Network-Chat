cmake_minimum_required(VERSION 3.16)
project(Net_chat)

set(CMAKE_CXX_STANDARD 17)

add_library(Net_chat net_message.cpp net_message.h net_common.h net_tsqueue.h net_connection.h net_server.h net_client.h olc_net.h)