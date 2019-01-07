cmake_minimum_required(VERSION 3.13)
project(Network7)

set(CMAKE_CXX_STANDARD 17)

add_executable(Network7 proxy.cpp SocksProxy.cpp)

target_link_libraries(Network7 anl)