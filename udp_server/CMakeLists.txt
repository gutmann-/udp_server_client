cmake_minimum_required(VERSION 3.23)
project(udp_server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

add_executable(udp_server
        main.cpp
        udp_server/packet.h
        udp_server/packet.cpp
        udp_server/base/buffer_reader.h
        udp_server/base/buffer_reader.cpp
        udp_server/base/buffer_writer.h
        udp_server/base/buffer_writer.cpp
        udp_server/base/sys_byteorder.h
        udp_server/net/socket.h
        udp_server/net/socket.cpp
        udp_server/net/address.h
        udp_server/net/ipv4_address.h
        udp_server/net/ipv4_address.cpp
        udp_server/net/address.cpp
        udp_server/net/udp_socket.h
        udp_server/net/udp_socket.cpp
        udp_server/server.h
        udp_server/server.cpp
        udp_server/file.h
        udp_server/file.cpp
        udp_server/base/crc32.h)
target_include_directories(udp_server PRIVATE ${CMAKE_SOURCE_DIR})