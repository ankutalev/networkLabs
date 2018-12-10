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

    void registerDescryptorForPollWrite(pollfd *desc);


    void readData(pollfd *from);

    void sendData(pollfd *to);

    void acceptConnection(int *newClient);

    void pollManage();

    bool connectToTarget();


    void removeFromPoll();


    void reBase();

private:
    static const int MAX_CLIENTS = 1024;
    static const int POLL_DELAY = 5000;
    static const int BUFFER_SIZE = 1500;
    char buff[BUFFER_SIZE];
    int port = 8081;
    int targetPort = 80;
    int mySocket = -1;
    int targetSocket = -1;
    sockaddr_in targetInfo;
    sockaddr_in forwarderInfo;
    std::string targetPath = "fit.ippolitov.me";
    std::vector<pollfd> *pollDescryptors = new std::vector<pollfd>;
    std::unordered_map<pollfd *, pollfd *> *transferPipes = new std::unordered_map<pollfd *, pollfd *>;
    std::unordered_map<pollfd *, std::vector<char> > *dataPieces = new std::unordered_map<pollfd *, std::vector<char>>;
    std::set<pollfd *> brokenDescryptors;
};