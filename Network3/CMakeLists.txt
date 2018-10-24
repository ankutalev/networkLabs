cmake_minimum_required(VERSION 3.12)
project(network3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS -lpthread)
add_executable(network3 main.cpp TreeNode.cpp TreeNode.h Socket/MySocket.cpp Socket/MySocket.h)
target_link_libraries(network3 uuid)