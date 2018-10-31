#include<stdio.h>	//printf
#include<string.h>	//strlen
#include<sys/socket.h>	//socket
#include<arpa/inet.h>	//inet_addr
#include <unistd.h>
#include <algorithm>
#include <sys/stat.h>
#include <iostream>
#include <fstream>

int main(int argc , char *argv[]) {
    int sock;
    struct sockaddr_in server;

    if (argc < 2) {
        std::cerr << "No file given!" << std::endl;
        return -1;
    }

    static const int MAX_FILE_PATH_IN_UTF_8 = 4 * 4096;
    static const int SERVER_REPLY_BUFFER_SIZE = 2000;

    char filePath[MAX_FILE_PATH_IN_UTF_8];
    char serverReply[SERVER_REPLY_BUFFER_SIZE];

    std::fill(filePath, filePath + MAX_FILE_PATH_IN_UTF_8, 0);
    std::fill(serverReply, serverReply + SERVER_REPLY_BUFFER_SIZE, 0);


    sock = socket(AF_INET , SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Could not create socket" << std::endl;
        return -1;
    }
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(528491);
    std::cout << "Trying to connect!" << std::endl;

    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        std::cerr << "Connect failed" << std::endl;
        return 1;
    }
    std::cout << "Connected" << std::endl;


    struct stat st;
    stat(argv[1], &st);
    std::string message(filePath);
    auto fileSize = std::to_string(st.st_size);
    fileSize += " ";
    fileSize += "LOLKEK ";
    message += fileSize;

    if (send(sock, message.c_str(), message.size(), 0) < 0) {
        std::cerr << "Send failed" << std::endl;
        return -1;
    }

    std::cout << recv(sock, serverReply, SERVER_REPLY_BUFFER_SIZE, 0);
    std::cout << serverReply;

    FILE* fin = fopen("heh.txt","r");
    size_t readed = SERVER_REPLY_BUFFER_SIZE;
    for (long long i = 0; readed == SERVER_REPLY_BUFFER_SIZE; ++i) {
        readed = fread(filePath, sizeof(char), SERVER_REPLY_BUFFER_SIZE, fin);
        send(sock, filePath, readed, 0);
    }
    std::fill(serverReply, serverReply + SERVER_REPLY_BUFFER_SIZE, 0);
    recv(sock, serverReply, SERVER_REPLY_BUFFER_SIZE, 0);
    std::cout << "Server reply: " << serverReply << std::endl;
    close(sock);
    fclose(fin);
    return 0;
}