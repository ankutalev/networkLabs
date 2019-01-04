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


static void registerForWrite(pollfd* fd) {
    fd->events = POLLOUT;
}


static void removeFromPoll(std::vector<pollfd>::iterator* it) {
    if (close((*it)->fd)) {
        std::cout << it << std::endl;
        std::cout << (*it)->fd << std::endl;
        perror("close");
    }
    (*it)->fd = -(*it)->fd;
}


void CacheProxy::writeToClient(std::vector<pollfd>::iterator* clientIterator) {
    std::cout << " TO DO";
    removeFromPoll(clientIterator);
}

void CacheProxy::passIdentification(std::vector<pollfd>::iterator* clientIterator) {
    pollfd* client = &**clientIterator;
    std::fill(buffer, buffer + BUFFER_LENGTH, 0);
    auto readed = recv(client->fd, buffer, BUFFER_LENGTH, 0);
    std::cout << "read in id" << readed << std::endl;
    buffer[readed] = 0;

    if (!checkSOCKSRequest(client->fd, readed)) {
        removeFromPoll(clientIterator);
        return;
    }

    char socksVersion = buffer[0];
    char socksCommand = buffer[1];
    char addrType = buffer[3];

    sockaddr_in targetAddr;
    targetAddr.sin_port = constructPort(buffer + 8);
    targetAddr.sin_addr.s_addr = constructIpAddr(buffer + 4);
    targetAddr.sin_family = AF_INET;


}

void CacheProxy::makeHandshake(std::vector<pollfd>::iterator* clientIterator) {
    pollfd* fd = &**clientIterator;
    auto readed = recv(fd->fd, buffer, BUFFER_LENGTH, 0);
    buffer[readed] = 0;
    printSocksBuffer(readed);
    char response[2] = {SOCKS_VERSION, INVALID_AUTHORISATION};
    int availableAuthorisations = buffer[1];
    for (int j = 0, i = 2; j < availableAuthorisations; ++i, ++j) {
        if (buffer[i] == SUPPORTED_AUTHORISATION) {
            std::cout << "supported auto! " << std::endl;
            response[1] = 0;
        }
    }
    send(fd->fd, response, HANDSHAKE_LENGTH, 0);

    auto addr = descToAddress[fd];
    waitingForAuthorisaton.erase(fd);
    passedAuthorisation.insert(addr);
}


void CacheProxy::acceptConnection(pollfd* client) {
    size_t addSize = sizeof(addr);
    int newClient = accept(serverSocket, (sockaddr*) &addr, (socklen_t*) &addSize);

    std::cout << "I ACCEPTED NEW CLIENT AND FD IS " << newClient << " " << inet_ntoa(addr.sin_addr) << " "
              << addr.sin_port << std::endl;

    if (newClient == -1) {
        throw std::runtime_error("can't accept!");
    }
    socklen_t size = sizeof(serverAddr);
    if (not doIKnowOwnIp) {
        getsockname(serverSocket, (sockaddr*) &serverAddr, &size);
        std::cout << inet_ntoa(serverAddr.sin_addr) << std::endl;
        doIKnowOwnIp = true;
    }
    //is this required?
    fcntl(newClient, F_SETFL, fcntl(newClient, F_GETFL, 0) | O_NONBLOCK);
    client->fd = newClient;
}

void CacheProxy::sendData(std::vector<pollfd>::iterator* target) {
    std::cout << " todo send data!" << std::endl;
    return;
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

    for (auto it = pollDescryptors->begin(); it != pollDescryptors->end(); ++it) {
        //если валидный
        std::cout << it->fd << " " << it->events << " " << it->revents << std::endl;
        if (it->fd > 0) {
            //если ошибка (конекшен рефьюзед)
            if (it->revents & POLLERR) {
                pollfd* client = (*transferMap)[&*it];
                close(client->fd);
                client->fd = (-client->fd);
                removeFromPoll(&it);
                std::cout << "REFUSED" << std::endl;
            } else if (it->revents & POLLOUT) {
                writeToClient(&it);
            } else if (it->revents & POLLIN) {
                if (it->fd == serverSocket) {
                    std::cout << "connection " << std::endl;
                    acceptConnection(&c);
                } else if (waitingForAuthorisaton.count(&*it)) {
                    std::cout << "handshake " << std::endl;
                    makeHandshake(&it);
                } else if (passedAuthorisation.count(descToAddress[&*it])) {
                    std::cout << "authorise" << std::endl;
                    passIdentification(&it);
                }
            }
        }
    }

    if (c.fd != -1) {
        pollDescryptors->push_back(c);
        SockAddrWrapper sock;
        sock.addr = addr;
        auto fd = &*(pollDescryptors->end() - 1);
        descToAddress[&*(pollDescryptors->end() - 1)] = sock;
        waitingForAuthorisaton.insert(&*(pollDescryptors->end() - 1));
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

    std::unordered_map<pollfd*, SockAddrWrapper> newDescToAddr;
    std::set<pollfd*> newWaiting;


    for (auto &pollDescryptor : *pollDescryptors) {
        if (pollDescryptor.fd > 0) {
            npollDescryptors->push_back(pollDescryptor);
            oldNewMap[&pollDescryptor] = &npollDescryptors->back();
        }
        if (waitingForAuthorisaton.count(&pollDescryptor))
            newWaiting.insert(&npollDescryptors->back());

    }


    for (auto &it : *transferMap) {
        if (it.second->fd > 0 and it.first->fd > 0) {
            (*ntransferPipes)[oldNewMap[it.first]] = oldNewMap[it.second];
            (*ntransferPipes)[oldNewMap[it.second]] = oldNewMap[it.first];
        }
    }


    for (auto &dataPiece : *dataPieces) {
        if (dataPiece.first->fd > 0)
            (*ndataPieces)[oldNewMap[dataPiece.first]] = dataPiece.second;
    }

    for (auto &fd : descToAddress) {
        if (fd.first->fd > 0)
            newDescToAddr[oldNewMap[fd.first]] = fd.second;
    }

    descToAddress.swap(newDescToAddr);


    dataPieces->swap(*ndataPieces);
    delete ndataPieces;

    transferMap->swap(*ntransferPipes);
    delete (ntransferPipes);

    pollDescryptors->swap(*npollDescryptors);
    delete npollDescryptors;

    newWaiting.swap(waitingForAuthorisaton);

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


