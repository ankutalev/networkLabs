#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>


int main(int argc, char* argv[]) {
    std::cout << "Hello, World!" << std::endl;
    const int port = 5284;
    WSAData wsaData;
    WSAStartup(MAKEWORD(2,2),&wsaData);
    char flag_on = 1;
    auto sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
// 
    if (sockfd==-1) {
        WSACleanup();
        std::cerr<<"Can't create socket! SeeGatLastError code"<<GetLastError()<<std::endl;
        return -1;
    }

    struct sockaddr_in addr, to;
    ZeroMemory(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr=INADDR_ANY;
    to.sin_family = AF_INET;
    to.sin_port = htons(528491);
    to.sin_addr.s_addr= inet_addr("224.0.1.15");
    if(bind(sockfd, (sockaddr *)&addr, sizeof(addr))) {

        std::cerr<<"Can't bind socket! SeeGatLastError code"<<GetLastError()<<std::endl;
        auto x = WSAGetLastError();
        std::cout<<x<<"\n";
        WSACleanup();
        return -1;
    }

    struct ip_mreq mreq;
//inet_pton(AF_INET,"224.0.1.15", &mreq.imr_multiaddr); 
    mreq.imr_multiaddr.s_addr = inet_addr("224.0.1.15");
//inet_pton(AF_INET,,&mreq.imr_interface);
//    mreq.imr_interface.s_addr = inet_addr("127.0.0.1"); //?>????????????
    mreq.imr_interface.s_addr = INADDR_ANY;


    std::cout<<setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
    std::string heh = "heha";
    std::cout<<WSAGetLastError()<<"\n";
    char buf [100500];
    int from = 0;
//std::cout<<recvfrom(sockfd, buf, 100500, 0, (struct sockaddr*) &addr, &from)<<"\n";
        std::cout<<sendto(sockfd, heh.c_str(), heh.size(), 0, (struct sockaddr*) &to, sizeof(addr))<<"\n";

        std::cout<<WSAGetLastError()<<"\n";
//std::cout << buf<<"\n"; 
//_sleep(2); 


    return 0;
}