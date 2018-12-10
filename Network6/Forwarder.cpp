#include "Forwarder.h"
#include <unistd.h>
#include <netdb.h>
#include <algorithm>

void Forwarder::acceptConnection(int *connect) {
    sockaddr_in clientAddr;
    int size = sizeof(clientAddr);

    *connect = accept(mySocket, (sockaddr * ) & clientAddr, (socklen_t *) &size);

    fcntl(*connect, F_SETFL, fcntl(*connect, F_GETFL, 0) | O_NONBLOCK);
    //after connectToTarget() call this->targetSocket updated

    if (dataPieces.count(*connect) or transferPipes.count(*connect)) {
        std::cout << "GOT YOU!!!" << std::endl;
        for (const auto &transferPipe : transferPipes) {
            if (transferPipe.second == *connect) {
                sendData(transferPipe.first);
                close(transferPipe.first);
                transferPipes.erase(transferPipe.first);
                for (auto it = pollDescryptors.begin();; ++it) {
                    if (it->fd == transferPipe.first) {
                        pollDescryptors.erase(it);
                        break;
                    }
                }
            }
        }
    }

    if (!connectToTarget()) {
        close(*connect);
        return;
    }


    char info[100];
    inet_ntop(AF_INET, &clientAddr, info, 100);
    std::cout << "ACCEPTED descryptor " << *connect << " from IP ADDR" << info << "IT'S TARGET SOCKEt IS "
              << targetSocket
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


    if (bind(mySocket, (sockaddr * ) & forwarderInfo, sizeof(forwarderInfo))) {
        throw std::runtime_error("Can't bind server socket!");
    }

    listen(mySocket, MAX_CLIENTS);
    std::fill(buff, buff + BUFFER_SIZE, 0);

    pollfd me;
    me.fd = mySocket;
    me.events = POLLIN;
    pollDescryptors.emplace_back(me);
    descryptorsID[nextId] = &pollDescryptors[0];
    nextId++;

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
    close(mySocket);
}


void Forwarder::startListen() {
    while (1)
        pollManage();
}

void Forwarder::pollManage() {

    std::cout << std::endl << std::endl << "BEFORE POLL" << std::endl << std::endl;
    for (const auto &descryptor : pollDescryptors) {
        std::cout << descryptor.fd << " events" << descryptor.events << " revents" << descryptor.revents << std::endl;
    }
    std::cout << "AND TRANSFER MAP IS " << std::endl;
    for (const auto &item : transferPipes) {
        std::cout << item.first << " " << item.second << std::endl;
//        if (abs(item.first - item.second) != abs(1)) {
//            std::cout << "AAAAAAAAAAAAAAAAAAa";
//            exit(100);
//        }

    }

    std::cout << "AND DATA IS" << std::endl;
    for (const auto &piece : dataPieces) {
        std::cout << piece.first << " " << piece.second.size() << std::endl;
    }


    poll(pollDescryptors.data(), pollDescryptors.size(), POLL_DELAY);

    std::cout << std::endl << std::endl << "AFTER POLL" << std::endl << std::endl;
    for (const auto &descryptor : pollDescryptors) {
        std::cout << descryptor.fd << " events" << descryptor.events << " revents" << descryptor.revents << std::endl;
    }

    for (auto it = pollDescryptors.begin(); it != pollDescryptors.end(); ++it) {
        int newClient = -1;
        std::cout << "ITERATOR TAKOY" << it->fd << std::endl;
        if (it->revents & POLLOUT) {
            sendData(it->fd);
        } else if (it->revents & POLLIN) {
            if (it->fd == mySocket) {
                acceptConnection(&newClient);
            } else {
                readData(it->fd);
            }
        }
        if (newClient != -1) {
            pollfd c;
            c.fd = targetSocket;
            c.events = POLLIN;
            it = pollDescryptors.insert(pollDescryptors.end() - 1, c);
            c.fd = newClient;
            it = pollDescryptors.insert(pollDescryptors.end() - 1, c);
            transferPipes[newClient] = targetSocket;
            transferPipes[targetSocket] = newClient;
            ++it;
            continue;
        }
    }

    removeFromPoll();

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


void Forwarder::readData(int from) {
    if (!isAvailableDescryptor(from)) {
        return;
    }
    int to = transferPipes[from];


    while (1) {
        auto readed = recv(from, buff, BUFFER_SIZE - 1, 0);
        buff[readed] = 0;
        std::cout << readed << "from " << from << std::endl;
        if (!readed) {
            if (from == 4) {
                std::cout << "SDOH PERVIY SOCKET!!!!" << std::endl;
            }
            std::cout << "REGAU " << from << std::endl;
            registerDescryptorForPollWrite(to);
            brokenDescryptors.insert(from);
            return;
        }
        if (errno == EWOULDBLOCK) {
            errno = EXIT_SUCCESS;
            std::cout << " AND RAZMER SCHITANNOY DATA is " << dataPieces[from].size() << std::endl;
            registerDescryptorForPollWrite(to);
            return;
        }
        for (int i = 0; i < readed; ++i) {
            dataPieces[from].emplace_back(buff[i]);
        }
    }

}

void Forwarder::sendData(int to) {
    int from = transferPipes[to];
    if (!dataPieces.count(from)) {
        return;
    }
    ssize_t sended = 0;
    auto size = dataPieces[from].size();
    do {
        sended = send(to, dataPieces[from].data(), dataPieces[from].size(), 0);
        std::cout << "RAZMER DATY IS " << dataPieces[from].size() << std::endl;
        std::cout << "SENDED IS" << sended;
//        std::cout << "DO ERAZE " << std::endl << dataPieces[from].data() << std::endl;
        dataPieces[from].erase(dataPieces[from].begin(), dataPieces[from].begin() + sended);
    } while (sended > 0);


//    std::cout << "POSLE ERAZE" << std::endl << dataPieces[from].data() << std::endl;
    std::cout << "i wrote this to " << to << std::endl;

    dataPieces.erase(from);
    for (auto it = pollDescryptors.begin(); it != pollDescryptors.end();) {
        if (it->fd == to) {
            it->events ^= POLLOUT;
            return;
        } else ++it;
    }
//    perror("wah");
//    if (to != 5)
//        exit(4);
}

void Forwarder::registerDescryptorForPollRead(int desc, sockaddr_in *addr, bool isClient) {

    pollfd c;
    c.fd = desc;
    c.events = POLLIN;
    pollDescryptors.emplace_back(c);

    if (isClient) {
        transferPipes[targetSocket] = c.fd;
        transferPipes[c.fd] = targetSocket;
    }


}

void Forwarder::unregDescryptorForReadFromPoll(int des) {
    for (auto it = pollDescryptors.begin(); it != pollDescryptors.end();) {
        if (it->fd == des) {
            it->events ^= POLLIN;
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

void Forwarder::registerDescryptorForDeleteFromPoll(int desc) {

}

void Forwarder::removeFromPoll() {
    for (const auto &item : brokenDescryptors) {
        std::cout << "ZDOhLI " << item << std::endl;
    }

    for (auto it = pollDescryptors.begin(); it != pollDescryptors.end();) {
        if (brokenDescryptors.count(it->fd)) {
            close(it->fd);
            std::cout << "CLOSE this! " << it->fd << std::endl;
            int target = transferPipes[it->fd];
            if (!target)
                dataPieces.erase(it->fd);
            transferPipes.erase(it->fd);
            brokenDescryptors.erase(it->fd);
            it = pollDescryptors.erase(it);
        } else ++it;
    }
}

bool Forwarder::isAvailableDescryptor(int desc) {
    for (const auto &item : pollDescryptors) {
        if (item.fd == desc)
            return true;
    }
    return false;
}





