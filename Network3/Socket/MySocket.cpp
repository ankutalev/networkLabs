#include <stdexcept>
#include <cstring>

#ifdef linux
#include <unistd.h>

#else
#include <in6addr.h>
#endif
#include "MySocket.h"
#include <algorithm>
#include <iostream>


MySocket::MySocket(int port) : MySocket(port, DEFAULT_IP_ADDR) {}

MySocket::MySocket(int port, std::string_view ipaddr) {
    this->port = port;
    this->ipAddress = ipaddr;

    if (!inet_pton(AF_INET, ipAddress.c_str(), &addrIPv4.sin_addr)) {
        protocol = AF_INET6;
        if (!inet_pton(AF_INET6, ipAddress.c_str(), &addrIPv6.sin6_addr))
            throw std::logic_error("Not IPv4 or IPv6 address!");
    }

    if (protocol == AF_INET) {
        addrIPv4.sin_family = protocol;
        addrIPv4.sin_port = htons(port);
    }
    else {
        addrIPv6.sin6_flowinfo = 0;
        addrIPv6.sin6_scope_id = 0;
        addrIPv6.sin6_family = protocol;
        addrIPv6.sin6_port = htons(port);
    }
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
    sockaddr* needed = protocol == AF_INET ? (sockaddr*) &sock.addrIPv4 : (sockaddr*) &sock.addrIPv6;
    auto size = protocol == AF_INET ? sizeof(sock.addrIPv4) : sizeof(sock.addrIPv6);
    res = sendto(descryptor, h.c_str(), msgSize, 0, needed, size);
    return res != -1;
}

std::string MySocket::read(const MySocket &sock) {
    auto size = protocol == AF_INET ? sizeof(sock.addrIPv4) : sizeof(sock.addrIPv6);
    sockaddr* needed = protocol == AF_INET ? (sockaddr*) &sock.addrIPv4 : (sockaddr*) &sock.addrIPv6;
    long long readed = 0;
    std::string msg;
    do {
        readed = recvfrom(descryptor, buffer, DEFAULT_BUFFER_SIZE, 0, needed, (socklen_t*) &size);
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
    auto size = protocol == AF_INET ? sizeof(addrIPv4) : sizeof(addrIPv6);
    sockaddr* needed = protocol == AF_INET ? (sockaddr*) &addrIPv4 : (sockaddr*) &addrIPv6;
    if (::bind(descryptor, needed, size)) {
        std::string errorMsg = "Can't bind! nodeSocket!" + std::string(strerror(errno)) + "\n";
        throw std::runtime_error(errorMsg);
    }
}


std::string MySocket::ipaddrAndPort() const {
    void* needed = protocol == AF_INET ? (void*) &addrIPv4.sin_addr : (void*) &addrIPv6.sin6_addr;
#ifndef linux
    inet_ntop(protocol, needed, (PSTR) info, IPV6_ADDR_SIZE_IN_CHAR);
#else
    inet_ntop(protocol, needed, (char *) info, IPV6_ADDR_SIZE_IN_CHAR);
#endif
    return std::string(info) + std::string(":/") + std::to_string(port);
}
std::string MySocket::getIpAddr() const {
    void* needed = protocol == AF_INET ? (void*) &addrIPv4.sin_addr : (void*) &addrIPv6.sin6_addr;
#ifndef linux
    inet_ntop(protocol, needed, (PSTR) info, IPV6_ADDR_SIZE_IN_CHAR);
#else
    inet_ntop(protocol, needed, (char *) info, IPV6_ADDR_SIZE_IN_CHAR);
#endif
    return info;
}

bool MySocket::joinMulticastGroup(std::string_view grAddr) {
//#ifndef linux
    if (protocol == AF_INET)
        addrIPv4.sin_addr.s_addr = htonl(INADDR_ANY);
    else
        addrIPv6.sin6_addr = in6addr_any;
//#endif

    this->bind();

    struct ip_mreq mreq;
    struct ipv6_mreq group;
    if (protocol == AF_INET) {
        inet_pton(protocol, grAddr.data(), &mreq.imr_multiaddr.s_addr);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    }
    else {
        group.ipv6mr_interface = htonl(INADDR_ANY);
    }

    int opt = 0;

    if (protocol == AF_INET) {
        return !setsockopt(descryptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mreq, sizeof(mreq))
               ? !setsockopt(
            descryptor,
            IPPROTO_IP,
            IP_MULTICAST_LOOP,
            (char*) &opt,
            sizeof(opt)) : false;
    }
    else {
        return !setsockopt(descryptor, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char*) &group, sizeof(group)) ? !setsockopt(
                descryptor,
                IPPROTO_IPV6,
                IPV6_MULTICAST_LOOP,
                (char*) &opt,
                sizeof(opt)) : false;
    }
}

bool MySocket::setRecvTimeOut(int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    return !setsockopt(descryptor, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout));
}

bool MySocket::setIpAddr(std::string_view adr) {
    ipAddress = adr;
    if (protocol == AF_INET)
        return inet_pton(protocol, ipAddress.c_str(), &addrIPv4.sin_addr) == 1;
    else
        return inet_pton(protocol, ipAddress.c_str(), &addrIPv6.sin6_addr) == 1;
}