#include <stdexcept>
#include <cstring>

#ifdef linux
#include <unistd.h>
#endif
#include "MySocket.h"
#include <algorithm>
#include <iostream>

MySocket::MySocket(int port) : MySocket(port, DEFAULT_IP_ADDR) {}

MySocket::MySocket(int port, std::string_view ipaddr) {
    this->port = port;
    this->ipAddress = ipaddr;

    char tmpIpAddrBuffer[IPV6_ADDR_SIZE_IN_CHAR];
    std::fill(tmpIpAddrBuffer, tmpIpAddrBuffer + IPV6_ADDR_SIZE_IN_CHAR, 0);


    if (!inet_pton(AF_INET, ipAddress.c_str(), tmpIpAddrBuffer)) {
        protocol = AF_INET6;
        if (!inet_pton(AF_INET6, ipAddress.c_str(), tmpIpAddrBuffer))
            throw std::logic_error("Not IPv4 or IPv6 address!");
    }

    //addr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
    inet_pton(protocol, ipAddress.c_str(), &addr.sin_addr);
    addr.sin_family = protocol;
    addr.sin_port = htons(port);
    std::fill(buffer, buffer + DEFAULT_BUFFER_SIZE, 0);
    std::fill(info, info + IPV6_ADDR_SIZE_IN_CHAR, 0);
#ifndef linux
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

}

MySocket::MySocket(std::string_view ipaddr) : MySocket(DEFAULT_PORT, ipaddr) {}

MySocket::MySocket() : MySocket(DEFAULT_PORT, DEFAULT_IP_ADDR) {}

MySocket::~MySocket() {
#ifdef linux
    close(descryptor);
#else
    closesocket(descryptor);
    WSACleanup();
#endif

}

bool MySocket::write(const MySocket &sock, std::string_view msg) {
    std::string h(msg);
    unsigned long msgSize = h.size();
    long long res = msgSize;
    res = sendto(descryptor, h.c_str(), msgSize, 0, (const sockaddr *) &sock.addr, sizeof(sock.addr));
    return res != -1;
}

std::string MySocket::read(const MySocket &sock) {
    auto size = sizeof(sock.addr);
    long long readed = 0;
    std::string msg;
    do {
        readed = recvfrom(descryptor, buffer, DEFAULT_BUFFER_SIZE, 0, (sockaddr *) &sock.addr, (socklen_t *) &size);
        if (readed != -1)
            buffer[readed] = '\0';
        msg += buffer;
    } while (readed == DEFAULT_BUFFER_SIZE);
    if (readed == -1) {
#ifdef linux
        //perror("Can't read from socket\n");
#else
        //std::cerr<<"Can't read from socket : wsagetlasterror"<<WSAGetLastError()<<std::endl;
#endif
        throw std::runtime_error("Read from nodeSocket failed!\n");
    }
    return msg;
}

void MySocket::open() {
    descryptor = socket(protocol, SOCK_DGRAM, IPPROTO_UDP);
    if (descryptor == -1) {
        std::string errorMsg = "Can't create nodeSocket!" + std::string(strerror(errno)) + "\n";
        throw std::runtime_error(errorMsg);
    }
}

void MySocket::bind() {
#ifndef linux
    addr.sin_addr.s_addr = INADDR_ANY;
#endif
    if (::bind(descryptor, (sockaddr*) &addr, sizeof(addr))) {
        std::string errorMsg = "Can't bind! nodeSocket!" + std::string(strerror(errno)) + "\n";
        throw std::runtime_error(errorMsg);
    }
}


std::string MySocket::ipaddrAndPort() const {
    inet_ntop(protocol, (void*) &addr.sin_addr, (PSTR) info, IPV6_ADDR_SIZE_IN_CHAR);
    return std::string(info) + std::string(":/") + std::to_string(port);
}
std::string MySocket::getIpAddr() const {
    inet_ntop(protocol, (void*) &addr.sin_addr, (PSTR) info, IPV6_ADDR_SIZE_IN_CHAR);
    return info;
}

bool MySocket::joinMulticastGroup(std::string_view grAddr) {
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(grAddr.data());
    int opt = 0;
    mreq.imr_interface.s_addr = INADDR_ANY;
    if (protocol == AF_INET) {
        return !setsockopt(descryptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) ? !setsockopt(
            descryptor,
            IPPROTO_IP,
            IP_MULTICAST_LOOP,
            (char*) &opt,
            sizeof(opt)) : false;
    }
    else {
        return !setsockopt(descryptor, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) ? !setsockopt(
            descryptor,
            IPPROTO_IPV6,
            IPV6_MULTICAST_LOOP,
            (char*) &opt,
            sizeof(opt)) : false;;
    }
}

bool MySocket::setRecvTimeOut(int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    return (setsockopt(descryptor, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout)) < 0);
}

bool MySocket::setIpAddr(std::string_view adr) {
    ipAddress = adr;
    return inet_pton(protocol, ipAddress.c_str(), &addr.sin_addr) == 1;
}