#pragma once

#include <string_view>
#include <unordered_map>
#include <mutex>
#include "MySocket.h"

class ReplicateCounter {
public:
    explicit ReplicateCounter(std::string_view mltcAddr);
    void startWorking();
private:
    void aliveChecker();
private:
    static const int SENDING_INTERVAL = 1;
    const int ALIVE_DURATION = 3;
    std::unordered_map<std::string, std::chrono::time_point<std::chrono::system_clock>> replicatesInfo;
    MySocket socket, groupSocket;
    std::mutex mutex;
    std::string grAddr;
};


