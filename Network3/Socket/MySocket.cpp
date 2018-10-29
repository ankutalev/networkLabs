#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include "MySocket.h"
#include <algorithm>
#include <iostream>

MySocket::MySocket(int port) : MySocket(port, DEFAULT_IP_ADDR) {

}

MySocket::MySocket(int port, std::string_view ipaddr) {
#ifdef linux
    isLinux = true;
#else
    isLinux = false;
#endif
    this->port = port;
    this->ipAddress = ipaddr;
    inet_pton()
    addr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    std::fill(buffer, buffer + DEFAULT_BUFFER_SIZE, 0);
}

MySocket::MySocket(std::string_view ipaddr) : MySocket(DEFAULT_PORT, ipaddr) {}

MySocket::MySocket() : MySocket(DEFAULT_PORT, DEFAULT_IP_ADDR) {}

MySocket::~MySocket() {
    close(descryptor);
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
    std::fill(buffer, buffer + DEFAULT_BUFFER_SIZE, 0);
    do {
        std::fill(buffer, buffer + readed, 0);
        readed = recvfrom(descryptor, buffer, DEFAULT_BUFFER_SIZE, 0, (sockaddr *) &sock.addr, (socklen_t *) &size);
        msg += buffer;
    } while (readed == DEFAULT_BUFFER_SIZE);
    if (readed == -1) {
        perror("huy\n");
        throw std::runtime_error("Read from nodeSocket failed!\n");
    }
    return msg;
}

void MySocket::open() {
    descryptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (descryptor == -1) {
        std::string errorMsg = "Can't create nodeSocket!" + std::string(strerror(errno)) + "\n";
        throw std::runtime_error(errorMsg);
    }
}

void MySocket::bind() {
    if (::bind(descryptor, (sockaddr *) &addr, sizeof(addr))) {
        std::string errorMsg = "Can't bind! nodeSocket!" + std::string(strerror(errno)) + "\n";
        throw std::runtime_error(errorMsg);
    }
}

std::string MySocket::ipaddrAndPort() const {
    return ipAddress + std::to_string(addr.sin_port);
}
