#include<iostream>
#include <unistd.h>
#include <cstdio>
#include <stdexcept>
#include <netdb.h>
#include <cstring>
#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include "SocksProxy.h"




static void removeFromPoll(std::vector<pollfd>::iterator* it) {
    if (close((*it)->fd)) {
        std::cout << it << std::endl;
        std::cout << (*it)->fd << std::endl;
        perror("close");
    }
    (*it)->fd = -(*it)->fd;
}


void CacheProxy::passIdentification(std::vector<pollfd>::iterator* clientIterator) {
    pollfd* client = &**clientIterator;
    std::fill(buffer, buffer + BUFFER_LENGTH, 0);
    auto readed = recv(client->fd, buffer, BUFFER_LENGTH, 0);
    std::cout << "read in id " << readed << std::endl;
    buffer[readed] = 0;

    if (!checkSOCKSRequest(client->fd, readed)) {
        removeFromPoll(clientIterator);
        return;
    }

    char socksVersion = buffer[0];
    char socksCommand = buffer[1];
    char addrType = buffer[3];

    sockaddr_in targetAddr;
    socklen_t addrSize = sizeof(targetAddr);
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = constructPort(buffer + 8);
    targetAddr.sin_addr.s_addr = constructIpAddr(buffer + 4);
    std::cout << "target port " << targetAddr.sin_port << std::endl;
    std::cout << "target addr " << inet_ntoa(targetAddr.sin_addr) << std::endl;
    char response[] = {SOCKS_VERSION, SOCKS_SUCCESS, 0, IPv4_ADDRESS, 0, 0, 0, 0, 0, 0};
    auto target = socket(AF_INET, SOCK_STREAM, 0);
    if (target == -1) {
        std::cout << "INVALID IP ADDR OR PORT GET!" << std::endl;
        response[1] = SOCKS_SERVER_ERROR;
        send(client->fd, response, sizeof(response), 0);
        passedHandshake.erase(client);
        removeFromPoll(clientIterator);
        return;
    }
    fcntl(target, F_SETFL, fcntl(target, F_GETFL, 0) | O_NONBLOCK);
    if (connect(target, (sockaddr*) &targetAddr, addrSize) and errno != EINPROGRESS) {
        response[1] = HOST_NOT_REACHABLE;
        send(client->fd, response, sizeof(response), 0);
        passedHandshake.erase(client);
        removeFromPoll(clientIterator);
        return;
    }
    std::cout << "ya zaconnectilsya!" << std::endl;
    send(client->fd, response, sizeof(response), 0);
    pollfd fd;
    fd.fd = target;
    fd.events = POLLIN;
    fd.revents = 0;
    *clientIterator = pollDescryptors->insert(*clientIterator + 1, fd);
    passedHandshake.erase(client);
    passedFullSOCKSprotocol.insert(client);
    passedFullSOCKSprotocol.insert(&**clientIterator);
    (*transferMap)[client] = &**clientIterator;
    (*transferMap)[&**clientIterator] = client;
}

void CacheProxy::makeHandshake(std::vector<pollfd>::iterator* clientIterator) {
    pollfd* fd = &**clientIterator;
    auto readed = recv(fd->fd, buffer, BUFFER_LENGTH, 0);
    std::cout << "I READ IN HANDSHAKE" << readed << std::endl;
    buffer[readed] = 0;
//    printSocksBuffer(readed);f

    char response[2] = {SOCKS_VERSION, INVALID_AUTHORISATION};
    if (buffer[0] != SOCKS_VERSION) {
        send(fd->fd, response, HANDSHAKE_LENGTH, 0);
        removeFromPoll(clientIterator);
        return;
    }

    int availableAuthorisations = buffer[1];
    for (int j = 0, i = 2; j < availableAuthorisations; ++i, ++j) {
        if (buffer[i] == SUPPORTED_AUTHORISATION) {
            std::cout << "supported auto! " << std::endl;
            response[1] = SUPPORTED_AUTHORISATION;
        }
    }

    send(fd->fd, response, HANDSHAKE_LENGTH, 0);
    if (response[1] != SUPPORTED_AUTHORISATION) {
        removeFromPoll(clientIterator);
        return;
    }
    passedHandshake.insert(fd);
}


void CacheProxy::acceptConnection(pollfd* client) {
    size_t addSize = sizeof(addr);
    int newClient = accept(serverSocket, (sockaddr*) &addr, (socklen_t*) &addSize);

    std::cout << "I ACCEPTED NEW CLIENT AND FD IS " << newClient << " " << inet_ntoa(addr.sin_addr) << " "
              << addr.sin_port << std::endl;

    if (newClient == -1) {
        throw std::runtime_error("can't accept!");
    }
    //is this required?
    fcntl(newClient, F_SETFL, fcntl(newClient, F_GETFL, 0) | O_NONBLOCK);
    client->fd = newClient;
}

void CacheProxy::sendData(std::vector<pollfd>::iterator* clientIterator) {
    std::cout << " send data!" << std::endl;

    auto client = &**clientIterator;
    ssize_t sended = 0;
    auto size = (*dataPieces)[client].size();
    do {
        sended = send(client->fd, (*dataPieces)[client].data(), (*dataPieces)[client].size(), 0);
        (*dataPieces)[client].erase((*dataPieces)[client].begin(), (*dataPieces)[client].begin() + sended);
    } while (sended > 0);
    dataPieces->erase(client);
    for (auto it = pollDescryptors->begin(); it != pollDescryptors->end();) {
        if (&*it == client) {
            it->events ^= POLLOUT;
            return;
        } else ++it;
    }
}

CacheProxy::CacheProxy() {
    init(DEFAULT_PORT);
}

CacheProxy::CacheProxy(int port) : port(port) {
    init(port);
}

void CacheProxy::startWorking() {
    while (1)
        pollManage();
}


void CacheProxy::pollManage() {
    pollfd c;
    c.fd = -1;
    c.events = POLLIN;
    c.revents = 0;

    auto cd = poll(&(*pollDescryptors)[0], pollDescryptors->size(), POLL_DELAY);
//    std::cout<<cd <<std::endl;
    std::cout << "NEW POLL CYCLE!!!!!!!!!" << std::endl;
    for (auto it = pollDescryptors->begin(); it != pollDescryptors->end(); ++it) {
        //если валидный
        std::cout << it->fd << " " << it->events << " " << it->revents << std::endl;
        if (it->fd > 0) {
            //если ошибка (конекшен рефьюзед)
            if (it->revents & POLLERR) {
                removeFromPoll(&it);
                std::cout << "REFUSED" << std::endl;
            } else if (it->revents & POLLOUT) {
                sendData(&it);
            } else if (it->revents & POLLIN) {
                if (it->fd == serverSocket) {
                    std::cout << "connection " << std::endl;
                    acceptConnection(&c);
                } else if (passedFullSOCKSprotocol.count(&*it)) {

                    readData(&it);
                } else if (passedHandshake.count(&*it)) {
                    std::cout << "socks authorise " << std::endl;
                    passIdentification(&it);
                    std::cout << "pass" << std::endl;
                } else {
                    std::cout << "making handshake! " << std::endl;
                    makeHandshake(&it);
                }
            }
        }
    }

    if (c.fd != -1) {
        pollDescryptors->push_back(c);
        auto fd = &*(pollDescryptors->end() - 1);
    }

    removeDeadDescryptors();
}

CacheProxy::~CacheProxy() {
    close(serverSocket);
    delete this->transferMap;
    delete this->pollDescryptors;
}

void CacheProxy::removeDeadDescryptors() {
    auto npollDescryptors = new std::vector<pollfd>;
    npollDescryptors->reserve(MAXIMIUM_CLIENTS);

    auto ntransferPipes = new std::map<pollfd*, pollfd*>;
    auto ndataPieces = new std::map<pollfd*, std::vector<char> >;
    std::map<pollfd*, pollfd*> oldNewMap;

    std::unordered_set<pollfd*> newPassedHandshake;
    std::unordered_set<pollfd*> newPassedSocks;


    for (auto &pollDescryptor : *pollDescryptors) {
        if (pollDescryptor.fd > 0) {
            npollDescryptors->push_back(pollDescryptor);
            oldNewMap[&pollDescryptor] = &npollDescryptors->back();
        }
    }



    for (auto &it : *transferMap) {
        if (it.second->fd > 0 and it.first->fd > 0) {
            (*ntransferPipes)[oldNewMap[it.first]] = oldNewMap[it.second];
            (*ntransferPipes)[oldNewMap[it.second]] = oldNewMap[it.first];
        }
    }

    for (const auto &handshake : passedHandshake) {
        if (handshake->fd > 0)
            newPassedHandshake.insert(oldNewMap[handshake]);
    }

    for (const auto &sock : passedFullSOCKSprotocol) {
        if (sock->fd > 0)
            newPassedSocks.insert(oldNewMap[sock]);
    }


    for (auto &dataPiece : *dataPieces) {
        if (dataPiece.first->fd > 0)
            (*ndataPieces)[oldNewMap[dataPiece.first]] = dataPiece.second;
    }



    dataPieces->swap(*ndataPieces);
    delete ndataPieces;

    transferMap->swap(*ntransferPipes);
    delete (ntransferPipes);

    pollDescryptors->swap(*npollDescryptors);
    delete npollDescryptors;

    passedFullSOCKSprotocol.swap(newPassedSocks);
    passedHandshake.swap(newPassedHandshake);
}

void CacheProxy::init(int port) {

    signal(SIGPIPE, SIG_IGN);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    pollDescryptors = new std::vector<pollfd>;
    transferMap = new std::map<pollfd*, pollfd*>;
    dataPieces = new std::map<pollfd*, std::vector<char> >;


    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (serverSocket == -1)
        throw std::runtime_error("Can't open server socket!");

    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*) &serverAddr, sizeof(serverAddr))) {
        throw std::runtime_error("Can't bind server socket!");
    }


    if (listen(serverSocket, MAXIMIUM_CLIENTS))
        throw std::runtime_error("Can't listen this socket!");

    if (fcntl(serverSocket, F_SETFL, fcntl(serverSocket, F_GETFL, 0) | O_NONBLOCK) == -1) {
        throw std::runtime_error("Can't make server socket nonblock!");
    }

    pollfd me;
    me.fd = serverSocket;
    me.events = POLLIN;
    pollDescryptors->reserve(MAXIMIUM_CLIENTS);
    pollDescryptors->push_back(me);

}

void CacheProxy::printSocksBuffer(int size) {
    for (int i = 0; i < size; ++i) {
        std::cout << i << " " << (int) buffer[i] << std::endl;
    }
}

bool CacheProxy::checkSOCKSRequest(int client, ssize_t len) {
    if (len < MINIMUM_SOCKS_REQUEST_LENGTH) {
        char notSupported[] = {SOCKS_VERSION, PROTOCOL_ERROR, 0};
        send(client, notSupported, sizeof(notSupported), 0);
        return false;
    }
    printSocksBuffer(len);
    char socksVersion = buffer[0];
    char socksCommand = buffer[1];
    char addrType = buffer[3];

    if (socksVersion != SOCKS_VERSION) {
        char notSupported[] = {SOCKS_VERSION, PROTOCOL_ERROR, 0};
        send(client, notSupported, sizeof(notSupported), 0);
        std::cerr << "invalid socks request!" << std::endl;
        return false;
    }
    if (addrType != IPv4_ADDRESS) {
        char notSupported[] = {SOCKS_VERSION, ADDRESS_NOT_SUPPORTED, 0};
        send(client, notSupported, sizeof(notSupported), 0);
        return false;
    }
    if (socksCommand != SUPPORTED_OPTION) {
        char notSupported[] = {SOCKS_VERSION, OPTION_NOT_SUPPORTED, 0};
        send(client, notSupported, sizeof(notSupported), 0);
        return false;
    }

    return true;
}

uint16_t CacheProxy::constructPort(char* port) {
    return *reinterpret_cast<uint16_t*>(port);
}

uint32_t CacheProxy::constructIpAddr(char* port) {
    return *reinterpret_cast<uint32_t*>(port);
}

void CacheProxy::readData(std::vector<pollfd>::iterator* clientIterator) {

    auto client = &**clientIterator;
    std::cout << "READ DATA" << std::endl;

    if (!transferMap->count(client)) {
        removeFromPoll(clientIterator);
        return;
    }
    pollfd* to = (*transferMap)[client];

    while (1) {
        auto readed = recv(client->fd, buffer, BUFFER_LENGTH - 1, 0);
        std::cout << "i read " << readed << std::endl;
        if (!readed or (readed == -1 and errno != EWOULDBLOCK)) {
            std::cout << "UJOHU!!!!!" << std::endl;
            registerForWrite(to);
            removeFromPoll(clientIterator);
            return;
        }
        if (errno == EWOULDBLOCK) {
            errno = EXIT_SUCCESS;
            registerForWrite(to);
            return;
        }
        buffer[readed] = 0;
        for (int i = 0; i < readed; ++i) {
            (*dataPieces)[to].emplace_back(buffer[i]);
        }
    }
}


