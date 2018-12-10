#pragma once

#include <vector>
#include <set>
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

    explicit Forwarder(int myPort, std::string_view targetName, int targetPort);

    void startListen();

    ~Forwarder();

private:
    void registerDescryptorForPollRead(int desc, sockaddr_in *addr, bool isClient);

    void registerDescryptorForPollWrite(int desc);

    void registerDescryptorForDeleteFromPoll(int desc);

    void readData(int from);

    void sendData(int to);

    void acceptConnection(int *newClient);

    void pollManage();

    bool connectToTarget();

    void unregDescryptorForReadFromPoll(int desc);

    void removeFromPoll();

    bool isAvailableDescryptor(int desc);

private:
    static const int MAX_CLIENTS = 1024;
    static const int POLL_DELAY = 5000;
    static const int BUFFER_SIZE = 1500;
    char buff[BUFFER_SIZE];
    int port = 8081;
    int targetPort = 80;
    int mySocket = -1;
    int targetSocket = -1;
    unsigned int nextId = 0;
    sockaddr_in targetInfo;
    sockaddr_in forwarderInfo;
    std::string targetPath = "fit.ippolitov.me";
    std::vector<pollfd> pollDescryptors;
    std::unordered_map<int, pollfd *> descryptorsID;
    std::unordered_map<int, int> transferPipes;
    std::unordered_map<int, std::vector<char> > dataPieces;
    std::set<int> brokenDescryptors;
};