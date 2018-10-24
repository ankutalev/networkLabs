cmake_minimum_required(VERSION 3.12)
project(Network2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-lpthread")
add_executable(Network2 main.cpp Server.cpp)
add_executable(client Client.cpp)