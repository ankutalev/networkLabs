cmake_minimum_required(VERSION 3.12)
project(network)

set(CMAKE_CXX_STANDARD 14)

add_executable(network main.cpp)
add_executable(test test.cpp)
target_link_libraries(network  ws2_32)
target_link_libraries(test ws2_32)