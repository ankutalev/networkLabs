#pragma once
#include <netinet/in.h>
#include <string>
#include <mutex>
#include <unordered_map>

struct ClientInfo {
    explicit ClientInfo (std::string_view ipAddr) : ipAddress(ipAddr) {}
    ClientInfo() = default;
    long long int alreadyLoaded = 0;
    long long int currentLoaded = 0;
    std::string ipAddress;
};


class Server {
public:
    explicit Server(char* portAsString);
    ~Server();
    void startListening();

private:
    void printDownloadsInfo();
    void downloadFile(int clientSocket);
    void errorHandler(int sfd, std::string_view msg);
    bool readOneMessage(char *buf, int sfd, std::string_view errorMsg);
private:
    const std::string pathToDownloads = "uploads";
    static const int DEFAULT_PORT = 528491;
    static const int MAXIMUM_CLIENTS = 16;
    static const int DOWNLOAD_BUFFER_SIZE = 4096;
    const int INFO_RANGE_IN_SECONDS = 3;
    struct sockaddr_in serverAddr,clientAddr;
    int sizeOfClientAddr = sizeof(clientAddr);
    int serverSocket;
    std::unordered_map<int,ClientInfo> clientsInfo;
    std::mutex locker;
};


