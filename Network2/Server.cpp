#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <thread>
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include "Server.h"


Server::Server(char *portAsString) {
    int port = DEFAULT_PORT;
    try {
        port = std::stoi(portAsString);
    }
    catch (std::exception &exc) {
        std::cerr << "Invalid port argument! Running with default port"<<std::endl;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
        throw std::runtime_error("Can't open server socket!\n");

    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr *) &serverAddr, sizeof(serverAddr)))
        throw std::runtime_error("Can't bind server socket!\n");


    if (listen(serverSocket, MAXIMUM_CLIENTS))
        throw std::runtime_error("Can't listen this socket!\n");


    if (auto tmp = opendir(pathToDownloads.c_str())) {
        closedir(tmp);
    } else {
        if (mkdir(pathToDownloads.c_str(), 0770))
            throw std::runtime_error("Can't create uploads folder!\n");
    }
}



void Server::startListening() {
    std::thread speedChecker(&Server::printDownloadsInfo,this);
    speedChecker.detach();
    std::vector<std::thread> threads;
    while (1) {
        auto clientSocket = accept(serverSocket, (sockaddr *) &clientAddr, (socklen_t *) &sizeOfClientAddr);
        char buffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, buffer, INET_ADDRSTRLEN);
        std::string clientIp (buffer);
        ClientInfo clientInfo (clientIp);
        std::cout<<"Got connection from: "<<clientIp<<std::endl;
        clientsInfo[clientSocket] = clientInfo;
        threads.emplace_back(&Server::downloadFile,this,clientSocket);
        threads.back().detach();
    }

}

void Server::printDownloadsInfo() {
    long long int secondsCounter = 0;
    while (1) {
        std::this_thread::sleep_for(std::chrono::seconds(INFO_RANGE_IN_SECONDS));
        secondsCounter+=INFO_RANGE_IN_SECONDS;
        std::cout<<"Current downloads info:"<<std::endl;
        locker.lock();
        for (const auto &item :clientsInfo) {
            std::cout <<"ip address:" << item.second.ipAddress
            << " current speed: " <<item.second.currentLoaded/(double)secondsCounter
            << " average speed: "<< item.second.alreadyLoaded/(double)secondsCounter<<std::endl;
        }
        locker.unlock();
    }
}

void Server::downloadFile(int clientSocket) {

    char  buffer [DOWNLOAD_BUFFER_SIZE] = {0};
    std::string str(buffer);

    do {
        readOneMessage(buffer, clientSocket, "Can't read meta info!");
        str += buffer;
    } while (str.size()==DOWNLOAD_BUFFER_SIZE);

    unsigned long got = str.size();

    send(clientSocket,std::to_string(str.size()).c_str(),DOWNLOAD_BUFFER_SIZE,0);
    std::cout<< std::to_string(str.size()).c_str()<<std::endl;
    readOneMessage(buffer,clientSocket,"Validation error!");

    if (std::string(buffer)!="OK") {
        std::cerr<<" error getting meta info"<<std::endl;
        return;
    }


    std::stringstream ss(str);
    std::string tmp;
    std::vector<std::string> protocolParams;

    clientsInfo[clientSocket].currentLoaded = str.size();

    while (ss>>tmp) {
        std::cout<<tmp;
        protocolParams.emplace_back(tmp);
    }

    if (protocolParams.size()<3) {
        errorHandler(clientSocket,"invalid meta info!");
    }

    if (protocolParams[2]!="LOLKEK") {
        std::cerr<<"Invalid check word! Must be LOLKEK found: "<<protocolParams[2]<<std::endl;
        errorHandler(clientSocket,"invalid meta info!");
        return;
    }


    long long fileSize = 0;

    try {
        fileSize = std::stoll(protocolParams[1]);
    }
    catch (std::exception& exc) {
        errorHandler(clientSocket,"Invalid filed size");
    }
    const std::string successMessage = "OK!";

    auto x = send(clientSocket,successMessage.c_str(),successMessage.size(),0);
    std::cout<<"IM SEND  "<<x;
    if (x==-1) {
        errorHandler(clientSocket,"Can't send protocol message!");
        return;
    }


    std::cout<<"I WILL DOWNLOAD "<<protocolParams[0]<<" with size "<<fileSize<<std::endl;

    std::string fullPath = pathToDownloads+"//"+protocolParams[0];

    std::ofstream file;

    file.open(fullPath, std::fstream::out);

    clientsInfo[clientSocket].alreadyLoaded=0;
    for(long long i = 0; i< fileSize/ DOWNLOAD_BUFFER_SIZE + 1;i++) {
        readOneMessage(buffer, clientSocket, "Can't get file data!\n");
        file << buffer;
    }
    std::cout<<"dokachal!\n";


    std::cout<< clientsInfo[clientSocket].alreadyLoaded<<std::endl;
    if(clientsInfo[clientSocket].alreadyLoaded==fileSize) {
        std::string verdict;
        if (send(clientSocket,successMessage.c_str(),DOWNLOAD_BUFFER_SIZE,0)==-1)
            verdict = "Can't send OK! to client, but file successfully downloaded!\n";
        else
            verdict = "All right!";
        errorHandler(clientSocket,verdict);
        file.close();
    } else {
        file.close();
        errorHandler(clientSocket,"Something was bad,deleting data\n");
        remove(fullPath.c_str());
    }

}

void Server::errorHandler(int sfd, std::string_view msg) {
    std::cerr<<msg<<std::endl;
    close(sfd);
    locker.lock();
    clientsInfo.erase(sfd);
    locker.unlock();
}

bool Server::readOneMessage(char* buf, int sfd, std::string_view errorMsg) {
    static ssize_t  wasRead = DOWNLOAD_BUFFER_SIZE;
    static size_t sum = 0;

    std::fill(buf,buf+wasRead,0);
    wasRead=recv(sfd,buf,DOWNLOAD_BUFFER_SIZE,0);
    if (!wasRead || wasRead==-1) {
        wasRead=DOWNLOAD_BUFFER_SIZE;
        return false;
    }
    sum+=wasRead;

    locker.lock();
    clientsInfo[sfd].currentLoaded=wasRead;
    clientsInfo[sfd].alreadyLoaded+=wasRead;
    locker.unlock();

    return true;
}

Server::~Server() {
    for (const auto &item : clientsInfo) {
        close(item.first);
    }
    close(serverSocket);
}



