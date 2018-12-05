cmake_minimum_required(VERSION 3.12)
project(Network6)

set(CMAKE_CXX_STANDARD 17)

add_executable(Network6 main.cpp Forwarder.cpp Forwarder.h)