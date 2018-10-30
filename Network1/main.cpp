#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "MySocket.h"

int main(int argc, char* argv[]) {
    std::string ipAddr = "127.0.0.1";
    std::string mltc = "224.0.1.15";
    MySocket sock(52849, ipAddr), to;
    sock.open();
    sock.bind();
    sock.joinMulticastGroup(mltc);
    while (1) {
        auto message = sock.read(to);
        auto info = to.ipaddrAndPort();
        std::cout << message << " from: " << info << std::endl;
    }
    return 0;
}