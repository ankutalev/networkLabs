#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <inaddr.h>
#include "MySocket.h"

int main(int argc, char* argv[]) {
    std::string ipAddr = "127.0.0.1";
    std::string mltc = "224.0.1.15";
    MySocket sock(528491, ipAddr), to(52849, mltc);
    sock.open();
    sock.bind();
    sock.joinMulticastGroup(mltc);
    sock.write(to, "privat potomki\n");
}