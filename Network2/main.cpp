#include <iostream>
#include <thread>
#include "Server.h"



int main(int argc, char* argv[]) {
    Server server (argv[1]);
    server.startListening();
    return 0;
}