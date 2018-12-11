#include <iostream>
#include "Forwarder.h"


int main(int argc, char *argv[]) {
    Forwarder *forwarder;
    if (argc == 4) {
        forwarder = new Forwarder(std::stoi(argv[1]), argv[2], std::stoi(argv[3]));
    } else
        forwarder = new Forwarder();
    forwarder->startListen();


    return 0;
}