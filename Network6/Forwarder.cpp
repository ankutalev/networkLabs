#include "Forwarder.h"
#include <unistd.h>
#include <netdb.h>

void Forwarder::acceptConnection() {
    sockaddr_in clientAddr;
    int size = sizeof(clientAddr);

    auto client = accept(clientsDescryptors[0].fd, (sockaddr *) &clientAddr, (socklen_t *) &size);
    pollfd c;
    c.fd = client;
    c.events = POLLIN;
    clientsAddresses[client] = clientAddr;
    clientsDescryptors.emplace_back(c);

    char info[100];
    inet_ntop(AF_INET, &clientAddr, info, 100);
    std::cout << "ACCEPTED descryptor " << client << " from IP ADDR" << info << std::endl;
}

Forwarder::Forwarder(int myPort, std::string_view targetName, int tPort) : targetPath(targetName), port(myPort),
                                                                           targetPort(tPort) {

    mySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mySocket == -1)
        throw std::runtime_error("Can't open server socket!");
    int opt = 1;
    if (setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) or
        fcntl(mySocket, F_SETFL, fcntl(mySocket, F_GETFL, 0) | O_NONBLOCK) == -1)
        throw std::runtime_error("Can't make socket nonblocking!");


    forwarderInfo.sin_addr.s_addr = inet_addr("127.0.0.1");
    forwarderInfo.sin_family = AF_INET;
    forwarderInfo.sin_port = htons(port);


    if (bind(mySocket, (sockaddr *) &forwarderInfo, sizeof(forwarderInfo))) {
        throw std::runtime_error("Can't bind server socket!");
    }

    listen(mySocket, MAX_CLIENTS);

    pollfd me;
    me.fd = mySocket;
    me.events = POLLIN;
    clientsDescryptors.emplace_back(me);
    clientsAddresses[mySocket] = forwarderInfo;
}

Forwarder::~Forwarder() {
    close(mySocket);
}


void Forwarder::startListen() {
    while (1)
        pollManage();
}

void Forwarder::pollManage() {
    auto changedDescryptors = poll(clientsDescryptors.data(), clientsDescryptors.size(), POLL_DELAY);
    for (int i = 0; i < changedDescryptors; ++i) {
        for (auto &clientsDescryptor : clientsDescryptors) {
            if (clientsDescryptor.revents == POLLIN)
                acceptConnection();
        }
    }
}


Forwarder::Forwarder() : Forwarder(8080, "fit.ippolitov.me", 80) {}

bool Forwarder::connectToTarget() {
    addrinfo hints = {0};
    hints.ai_flags = 0;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo *addr = nullptr;
    if (getaddrinfo(targetPath.c_str(), nullptr, &hints, &addr)) {
        std::cerr << " Can't resolve hostname!!! Terminating" << std::endl;
        return false;
    }

    targetInfo = *(sockaddr_in *) (addr->ai_addr);
    targetInfo.sin_family = AF_INET;
    targetInfo.sin_port = htons(80);

    targetSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (targetSocket == -1) {
        std::cerr << "Can't open target socket! Terminating" << std::endl;
        return false;
    }
    if (fcntl(targetSocket, F_SETFL, fcntl(targetSocket, F_GETFL, 0) | O_NONBLOCK) == -1) {
        std::cerr << "Can't make target socket non blocking! Terminating!" << std::endl;
        return false;
    }
    if (connect(targetSocket, (sockaddr *) &targetInfo, sizeof(targetInfo))) {
        std::cerr << "Can't connect to target! Terminating!" << std::endl;
        return false
    }

    return true;
}





