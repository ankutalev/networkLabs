#pragma once
#include <string_view>
#include <string>

#ifdef linux
    #include<sys/socket.h>    //nodeSocket
    #include<arpa/inet.h>    //inet_addr
#else
    #include <ws2tcpip.h>
    #include <winsock2.h>
#endif
class MySocket {
public:
    explicit MySocket(int port);  // localhost
    MySocket(); // localhost with default port
    MySocket(int port, std::string_view ipaddr); // custom ip and port
    explicit MySocket(std::string_view ipaddr); // custom ip on default port
    ~MySocket();

    bool write(const MySocket &sock, std::string_view msg);

    void open();

    void bind();

    std::string ipaddrAndPort() const;

    std::string read(const MySocket &sock);

    bool operator<(const MySocket &rhs) const {
        return port < rhs.port;
    }

private:
    static const int DEFAULT_PORT = 528491;
    static const int DEFAULT_BUFFER_SIZE = 4096;
    constexpr static const char DEFAULT_IP_ADDR[] = "127.0.0.1";
    int port = 0;
    char buffer[DEFAULT_BUFFER_SIZE] = {0};
    struct sockaddr_in addr;
    int descryptor = -1;
    std::string ipAddress = DEFAULT_IP_ADDR;
    bool isLinux = true;
};


