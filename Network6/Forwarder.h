#pragma once

#include <vector>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unordered_map>
#include <iostream>

class Forwarder {
public:
    Forwarder();
    explicit Forwarder(int myPort,std::string_view targetName, int targetPort);
    void startListen();
    ~Forwarder();
private:
    void acceptConnection();
    void pollManage();
    bool connectToTarget();
private:
    static const int MAX_CLIENTS = 1024;
    static const int POLL_DELAY = 5000;
    int port = 8081;
    int targetPort;
    int mySocket= -1;
    int targetSocket = -1;
    sockaddr_in targetInfo;
    sockaddr_in forwarderInfo;
    std::string targetPath;
    std::vector<pollfd> clientsDescryptors;
    std::unordered_map<int,sockaddr_in> clientsAddresses;
};