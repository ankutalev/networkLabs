#pragma once

#include <string>
#include <set>
#include <mutex>
#include <unordered_map>
#include <condition_variable>
#include <fstream>
#include "Socket/MySocket.h"

class TreeNode {
public:
    TreeNode(int port, std::string_view name, double loss);

    TreeNode(int port, std::string_view name, double loss, std::string_view ancestor, int ancestorPort);

    void workBody();

private:
    void receiving();

    void sending();

    void sendingLostMessages();

private:
    MySocket nodeSocket, otherSocket;
    std::string name = "grisha";
    double procentOfLoss = 0;
    int port = 52849;
    std::string ancestor;
    int ancestorPort = 1488;
    std::unordered_map<std::string, MySocket> myAbonents; //ip+port -> socket
    std::unordered_map<std::string, std::string> messages; // uuid -> full message
    std::unordered_map<std::string, std::pair<std::string, bool>> messagesStatus; // uuid -> (ip+port, received)
    const std::string SUCCESS_MARKER = "LOLKEK";
    const int TIME_OUT = 3;
    static const int UUID_SIZE_BUF = 36;
    static const int SUCCESS_MARKER_LENGTH = UUID_SIZE_BUF + 6;
    std::mutex mutex;
    std::condition_variable cond_var;
    FILE *logFile = nullptr;
};


