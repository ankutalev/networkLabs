#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <poll.h>
#include <set>
#include <unordered_map>
#include <unordered_set>


class SocksProxy {
public:
    SocksProxy();

    ~SocksProxy();

    explicit SocksProxy(int port);

    void startWorking();

private:
    void pollManage();

    void removeDeadDescryptors();

    void init(int port);

    void connectToIPv4Address(std::vector<pollfd>::iterator* clientIterator);

    void connectToIPv6Address(std::vector<pollfd>::iterator* clientIterator);

    void resolveDomainName(std::vector<pollfd>::iterator* clientIterator);

    void getResolveResult(std::vector<pollfd>::iterator* clientIterator);

    void registerForWrite(pollfd* fd) { fd->events |= POLLOUT; }

    void passIdentification(std::vector<pollfd>::iterator* clientIterator);

    void sendData(std::vector<pollfd>::iterator* clientIterator);

    void acceptConnection(pollfd* client);

    void makeHandshake(std::vector<pollfd>::iterator* clientIterator);

    void readData(std::vector<pollfd>::iterator* clientIterator);

    bool checkSOCKSRequest(int client, ssize_t len);

    void printSocksBuffer(int size);

    uint32_t constructIPv4Addr(char* addr);

    in6_addr constructIPv6Addr(char* addr);

    uint16_t constructPort(char* port);


private:
    int port;
    int serverSocket;
    const static int MAXIMIUM_CLIENTS = 2048;
    const static int DEFAULT_PORT = 8080;
    const static int POLL_DELAY = 3000;
    const static int BUFFER_LENGTH = 5000;
    const static int HANDSHAKE_LENGTH = 2;
    const static char SOCKS_VERSION = 5;
    const static char SOCKS_SERVER_ERROR = 1;
    const static char INVALID_AUTHORISATION = 0xff;
    const static char SUPPORTED_AUTHORISATION = 0;
    const static char SUPPORTED_OPTION = 1;
    constexpr static char IPv4_ADDRESS = 1;
    constexpr static char IPv6_ADDRESS = 4;
    constexpr static char DOMAIN_NAME = 3;
    const static char OPTION_NOT_SUPPORTED = 7;
    const static char PROTOCOL_ERROR = 7;
    const static char SOCKS_SUCCESS = 0;
    const static char ADDRESS_NOT_SUPPORTED = 8;
    const static char HOST_NOT_REACHABLE = 4;
    const static int MINIMUM_SOCKS_REQUEST_LENGTH = 10;
    const static int IPv4_ADDRESS_LENGTH = 4;
    const static int IPv6_ADDRESS_LENGTH = 16;
    const static int SOCKS5_OFFSET_BEFORE_ADDR = 4;
    std::unordered_set<char> supportedAddressTypes;
    char buffer[BUFFER_LENGTH];
    sockaddr_in serverAddr;
    sockaddr_in addr;
    std::vector<pollfd>* pollDescryptors;
    std::unordered_set<pollfd*> passedHandshake;
    std::unordered_set<pollfd*> passedFullSOCKSprotocol;
    std::unordered_map<pollfd*, pollfd*>* transferMap;
    std::unordered_map<pollfd*, std::vector<char> >* dataPieces;
    std::unordered_map<pollfd*, pollfd*> waitedForDnsToResolvers;
};


