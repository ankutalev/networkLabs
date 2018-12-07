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
    void registerDescryptorForPollRead(int desc, sockaddr_in *addr, bool isClient);

    void registerDescryptorForPollWrite(int desc);

    void readData(int from);

    void sendData(int to);
    void acceptConnection();
    void pollManage();
    bool connectToTarget();

    void eraseFromPoll(int desc);
private:
    static const int MAX_CLIENTS = 1024;
    static const int POLL_DELAY = 5000;
    static const int BUFFER_SIZE = 50;
    char buff[BUFFER_SIZE];
    int port = 8081;
    int targetPort = 80;
    int mySocket= -1;
    int targetSocket = -1;
    sockaddr_in targetInfo;
    sockaddr_in forwarderInfo;
    std::string targetPath = "fit.ippolitov.me";
    std::vector<pollfd> pollDescryptors;
    std::unordered_map<int,sockaddr_in> clientsAddresses;
    std::unordered_map<int, int> transferPipes;
    std::unordered_map<int, std::string> dataPieces;
};