#include "Forwarder.h"
#include <unistd.h>
#include <netdb.h>
#include <algorithm>

void Forwarder::acceptConnection() {
    sockaddr_in clientAddr;
    int size = sizeof(clientAddr);

    auto client = accept(pollDescryptors[0].fd, (sockaddr *) &clientAddr, (socklen_t *) &size);
    fcntl(client, F_SETFL, fcntl(client, F_GETFL, 0) | O_NONBLOCK);
    //after connectToTarget() call this->targetSocket updated

    if (!connectToTarget()) {
        close(client);
        return;
    }

    registerDescryptorForPollRead(client, &clientAddr, true);
    registerDescryptorForPollRead(targetSocket, &targetInfo, false);

    char info[100];
    inet_ntop(AF_INET, &clientAddr, info, 100);
    std::cout << "ACCEPTED descryptor " << client << " from IP ADDR" << info << "IT'S TARGET SOCKEt IS " << targetSocket
              << std::endl;
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
    std::fill(buff, buff + BUFFER_SIZE, 0);
    pollfd me;
    me.fd = mySocket;
    me.events = POLLIN;
    pollDescryptors.emplace_back(me);
    clientsAddresses[mySocket] = forwarderInfo;


    addrinfo hints = {0};
    hints.ai_flags = 0;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo *addr = nullptr;
    if (getaddrinfo(targetPath.c_str(), nullptr, &hints, &addr)) {
        std::cerr << " Can't resolve hostname!!! Terminating" << std::endl;
    }

    targetInfo = *(sockaddr_in *) (addr->ai_addr);
    targetInfo.sin_family = AF_INET;
    targetInfo.sin_port = htons(80);


}

Forwarder::~Forwarder() {
    close(mySocket);
}


void Forwarder::startListen() {
    while (1)
        pollManage();
}

void Forwarder::pollManage() {
    auto changedDescryptors = poll(pollDescryptors.data(), pollDescryptors.size(), POLL_DELAY);
//    std::cerr<<"CHANGED DESCRYPTORS "<<changedDescryptors<<std::endl;
    for (int i = 0; i < changedDescryptors; ++i) {
        for (auto &clientsDescryptor : pollDescryptors) {
            if (clientsDescryptor.revents & POLLIN) {
                if (clientsDescryptor.fd == mySocket) {
                    acceptConnection();
                } else {
                    readData(clientsDescryptor.fd);
                }
            } else if (clientsDescryptor.revents & POLLOUT)
                sendData(clientsDescryptor.fd);
        }
    }
}


Forwarder::Forwarder() : Forwarder(8080, "fit.ippolitov.me", 80) {}

bool Forwarder::connectToTarget() {

    targetSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (targetSocket == -1) {
        std::cerr << "Can't open target socket! Terminating" << std::endl;
        return false;
    }
    if (connect(targetSocket, (sockaddr *) &targetInfo, sizeof(targetInfo))) {
        perror("WO \n");
        std::cerr << "Can't connect to target! Terminating!" << std::endl;
        return false;
    }
    if (fcntl(targetSocket, F_SETFL, fcntl(targetSocket, F_GETFL, 0) | O_NONBLOCK) == -1) {
        std::cerr << "Can't make target socket non blocking! Terminating!" << std::endl;
        return false;
    }

    return true;
}


void Forwarder::readData(int from) {
//    std::string data;
    int to = transferPipes[from];
    std::cerr << "READING";

//    while(1) {
    auto readed = recv(from, buff, BUFFER_SIZE - 1, 0);
    std::cerr << readed << " " << (errno == EWOULDBLOCK) << std::endl;
//        if (errno==EWOULDBLOCK)
//            break;
    if (!readed) {
        close(from);
        close(to);
        transferPipes.erase(to);
        transferPipes.erase(from);
        eraseFromPoll(from);
        eraseFromPoll(to);
        return;
    }
//        buff[readed] = 0;
//        data += buff;
//    }
//    std::cerr<<"Ready read";
//    registerDescryptorForPollWrite(to);
//    dataPieces[from] = data;
}

void Forwarder::sendData(int to) {
//    int from = transferPipes[to];
//    std::cerr<<"SENDING!\n";
//    std::cout<<send(to, dataPieces[from].c_str(), dataPieces[from].size(), 0);
//    std::cerr<<"sended!\n";
//    std::cout<<dataPieces[from]<<std::endl;
//    perror("wah");
    if (to != 5)
        exit(4);
}

void Forwarder::registerDescryptorForPollRead(int desc, sockaddr_in *addr, bool isClient) {

    pollfd c;
    c.fd = desc;
    c.events = POLLIN;
    pollDescryptors.emplace_back(c);

    if (isClient) {
        clientsAddresses[desc] = *addr;
        transferPipes[targetSocket] = c.fd;
        transferPipes[c.fd] = targetSocket;
    }


}

void Forwarder::eraseFromPoll(int des) {
    for (auto it = pollDescryptors.begin(); it != pollDescryptors.end();) {
        if (it->fd == des) {
            pollDescryptors.erase(it);
            return;
        } else ++it;
    }
}

void Forwarder::registerDescryptorForPollWrite(int desc) {
    for (auto it = pollDescryptors.begin(); it != pollDescryptors.end();) {
        if (it->fd == desc) {
            it->events |= POLLOUT;
            return;
        } else ++it;
    }
}





