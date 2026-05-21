cmake_minimum_required(VERSION 3.10)
project(netprobe)

set(CMAKE_CXX_STANDARD 17)

add_executable(netprobe
    src/main.cpp
    src/ping.cpp
    src/traceroute.cpp
)