#include "Forwarder.h"
#include <unistd.h>
#include <netdb.h>
#include <algorithm>

void Forwarder::acceptConnection(int *connect) {
    sockaddr_in clientAddr;
    int size = sizeof(clientAddr);

    *connect = accept(mySocket, (sockaddr * ) & clientAddr, (socklen_t *) &size);

    fcntl(*connect, F_SETFL, fcntl(*connect, F_GETFL, 0) | O_NONBLOCK);

    if (!connectToTarget()) {
        close(*connect);
        return;
    }


    char info[100];
    inet_ntop(AF_INET, &clientAddr, info, 100);
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


    if (bind(mySocket, (sockaddr * ) & forwarderInfo, sizeof(forwarderInfo))) {
        throw std::runtime_error("Can't bind server socket!");
    }

    listen(mySocket, MAX_CLIENTS);
    std::fill(buff, buff + BUFFER_SIZE, 0);

    pollfd me;
    me.fd = mySocket;
    me.events = POLLIN;
    pollDescryptors->emplace_back(me);
    pollDescryptors->reserve(MAX_CLIENTS);

    transferPipes->reserve(MAX_CLIENTS);

    addrinfo hints = {0};
    hints.ai_flags = 0;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo *addr = nullptr;
    if (getaddrinfo(targetPath.c_str(), nullptr, &hints, &addr)) {
        std::cout << " Can't resolve hostname!!! Terminating" << std::endl;
    }

    targetInfo = *(sockaddr_in *) (addr->ai_addr);
    targetInfo.sin_family = AF_INET;
    targetInfo.sin_port = htons(80);

}

Forwarder::~Forwarder() {
    delete pollDescryptors;
    delete transferPipes;
    delete dataPieces;
    close(mySocket);
}


void Forwarder::startListen() {
    while (1)
        pollManage();
}

void Forwarder::pollManage() {

    pollfd c;
    c.fd = -1;
    c.events = POLLIN;


    poll(pollDescryptors->data(), pollDescryptors->size(), POLL_DELAY);

    for (auto it = pollDescryptors->begin(); it != pollDescryptors->end(); ++it) {
        if (it->revents & POLLOUT) {
            sendData(&*it);
        } else if (it->revents & POLLIN) {
            if (it->fd == mySocket) {
                acceptConnection(&c.fd);
            } else {
                readData(&*it);
            }
        }
    }

    removeFromPoll();
    if (pollDescryptors->size() > 10)
        reBase();


    if (c.fd != -1) {
        pollDescryptors->push_back(c);
        c.fd = targetSocket;
        pollDescryptors->push_back(c);
        (*transferPipes)[&*(pollDescryptors->end() - 1)] = &*(pollDescryptors->end() - 2);
        (*transferPipes)[&*(pollDescryptors->end() - 2)] = &*(pollDescryptors->end() - 1);
    }


}


Forwarder::Forwarder() : Forwarder(8080, "fit.ippolitov.me", 80) {}

bool Forwarder::connectToTarget() {

    targetSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (targetSocket == -1) {
        std::cout << "Can't open target socket! Terminating" << std::endl;
        return false;
    }
    if (connect(targetSocket, (sockaddr * ) & targetInfo, sizeof(targetInfo))) {
        perror("WO \n");
        std::cout << "Can't connect to target! Terminating!" << std::endl;
        return false;
    }
    if (fcntl(targetSocket, F_SETFL, fcntl(targetSocket, F_GETFL, 0) | O_NONBLOCK) == -1) {
        std::cout << "Can't make target socket non blocking! Terminating!" << std::endl;
        return false;
    }

    return true;
}


void Forwarder::readData(pollfd *idDesc) {

    if (!transferPipes->count(idDesc)) {
        brokenDescryptors.insert(idDesc);
        return;
    }
    pollfd *to = (*transferPipes)[idDesc];

    while (1) {
        auto readed = recv(idDesc->fd, buff, BUFFER_SIZE - 1, 0);
        buff[readed] = 0;
        if (!readed) {
            registerDescryptorForPollWrite(to);
            brokenDescryptors.insert(idDesc);
            return;
        }
        if (errno == EWOULDBLOCK) {
            errno = EXIT_SUCCESS;
            registerDescryptorForPollWrite(to);
            return;
        }
        for (int i = 0; i < readed; ++i) {
            (*dataPieces)[to].emplace_back(buff[i]);
        }
    }

}

void Forwarder::sendData(pollfd *idDesc) {
    ssize_t sended = 0;
    auto size = (*dataPieces)[idDesc].size();

    do {
        sended = send(idDesc->fd, (*dataPieces)[idDesc].data(), (*dataPieces)[idDesc].size(), 0);
        (*dataPieces)[idDesc].erase((*dataPieces)[idDesc].begin(), (*dataPieces)[idDesc].begin() + sended);
    } while (sended > 0);
    dataPieces->erase(idDesc);
    for (auto it = pollDescryptors->begin(); it != pollDescryptors->end();) {
        if (&*it == idDesc) {
            it->events ^= POLLOUT;
            return;
        } else ++it;
    }
}


void Forwarder::registerDescryptorForPollWrite(pollfd *idDesc) {
    for (auto it = pollDescryptors->begin(); it != pollDescryptors->end();) {
        if (&*it == idDesc) {
            it->events |= POLLOUT;
            return;
        } else ++it;
    }
}


void Forwarder::removeFromPoll() {

    for (auto it = pollDescryptors->begin(); it != pollDescryptors->end();) {
        if (brokenDescryptors.count(&*it) and it->fd > 0) {
            close(it->fd);
            std::cout << "CLOSE this! " << it->fd << std::endl;
            it->fd = -it->fd;
        } else ++it;
    }
}


void Forwarder::reBase() {

    std::vector<pollfd> *npollDescryptors = new std::vector<pollfd>;
    npollDescryptors->reserve(MAX_CLIENTS);

    std::unordered_map<pollfd *, pollfd *> *ntransferPipes = new std::unordered_map<pollfd *, pollfd *>;
    ntransferPipes->reserve(MAX_CLIENTS);

    std::unordered_map<pollfd *, std::vector<char> > *ndataPieces = new std::unordered_map<pollfd *, std::vector<char> >;
    std::unordered_map<pollfd *, pollfd *> oldNewMap;

    for (auto it = pollDescryptors->begin(); it != pollDescryptors->end(); ++it) {
        if (it->fd > 0) {
            npollDescryptors->push_back(*it);
            oldNewMap[&*it] = &npollDescryptors->back();
        }
    }


    for (auto it = transferPipes->begin(); it != transferPipes->end(); ++it) {
        if (it->second->fd > 0 and it->first->fd > 0) {
            (*ntransferPipes)[oldNewMap[it->first]] = oldNewMap[it->second];
            (*ntransferPipes)[oldNewMap[it->second]] = oldNewMap[it->first];
        }
    }


    for (auto it = dataPieces->begin(); it != dataPieces->end(); ++it) {
        if (it->first->fd > 0)
            (*ndataPieces)[oldNewMap[it->first]] = it->
                    second;
    }


    brokenDescryptors.clear();

    delete dataPieces;
    dataPieces = ndataPieces;
    delete transferPipes;
    transferPipes = ntransferPipes;
    delete pollDescryptors;
    pollDescryptors = npollDescryptors;
    std::cout << "REBASED FINISHED\n with size " << pollDescryptors->size() << std::endl;

}





