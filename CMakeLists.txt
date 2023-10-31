cmake_minimum_required(VERSION 3.17)
project(Peer2PeerFileTransfer C)

set(CMAKE_C_STANDARD 99)

add_executable(Peer2PeerFileTransfer server.c client.c)