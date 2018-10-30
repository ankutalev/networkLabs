#include <iostream>
#include "ReplicateCounter.h"


int main(int argc, char* argv[]) {
    std::string localhost;
    std::string mltcs;
    if (argc < 2) {
        localhost = "127.0.0.1";
        mltcs = "224.0.1.15";
    } else {
        sockaddr_in addrIPv4Check;
        sockaddr_in6 addrIPv6Check;
        if (inet_pton(AF_INET, argv[1], &addrIPv4Check.sin_addr) == 1)
            localhost = "127.0.0.1";
        else if (inet_pton(AF_INET6, argv[1], &addrIPv6Check.sin6_addr) == 1) {
            localhost = "::1";
        } else {
            std::cerr << "Invalid multicast addr!" << std::endl;
            return 1;
        }
        mltcs = argv[1];
    }
    ReplicateCounter counter(localhost, mltcs);
    counter.startWorking();
    return 0;
}
//FF02::1:FF47:0
//224.0.1.15
