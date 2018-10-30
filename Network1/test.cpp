#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <inaddr.h>
#include <chrono>
#include <ctime>
#include "MySocket.h"

int main(int argc, char* argv[]) {
    std::string ipAddr = "127.0.0.1";
    std::string mltc = "224.0.1.15";
    MySocket sock(528491, ipAddr), to(52849, mltc);
    sock.open();
    sock.bind();
    sock.joinMulticastGroup(mltc);
    sock.write(to, "privat potomki\n");
    auto time = std::chrono::system_clock::now();
    auto time1 = std::chrono::system_clock::now();
    std::chrono::duration<double> timePassed = time - time1;
    std::cout << std::chrono::duration_cast<std::chrono::seconds>(time1 - time).count();
    //std::time_t t = std::chrono::system_clock::to_time_t(time);
    //std::cout << std::ctime(&t) << std::endl;

}