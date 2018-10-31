#include <iostream>
#include <thread>
#include "ReplicateCounter.h"

ReplicateCounter::ReplicateCounter(std::string_view lcl, std::string_view mltcAddr) : grAddr(mltcAddr),
    groupSocket(mltcAddr), socket(lcl) {
#ifdef linux
    socket = MySocket(mltcAddr);
#endif
}

void ReplicateCounter::aliveChecker() {
    while (1) {
        std::this_thread::sleep_for(std::chrono::seconds(ALIVE_DURATION));
        auto currentTime = std::chrono::system_clock::now();
        for (auto it = replicatesInfo.begin(); it != replicatesInfo.end();) {
            mutex.lock();
            auto lastTime = it->second;
            if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastTime).count() > ALIVE_DURATION) {
                std::cerr << "Another dead app: " << it->first << std::endl << "Now alive: " << std::endl;
                it = replicatesInfo.erase(it);
                for (const auto& alive : replicatesInfo) {
                    std::cerr << alive.first << std::endl;
                }
            }
            else
                ++it;
            mutex.unlock();
        }
    }
}
void ReplicateCounter::startWorking() {
    socket.open();
    socket.setRecvTimeOut(SENDING_INTERVAL);
    socket.joinMulticastGroup(grAddr);
    std::thread checker(&ReplicateCounter::aliveChecker, this);
    checker.detach();

    while (1) {
        socket.write(groupSocket, "LOLKEK");
        try {
            auto msg = socket.read(groupSocket);
        }
        catch (std::exception& e) {
            groupSocket.setIpAddr(grAddr);
            continue;
        }
        auto fromIp = groupSocket.getIpAddr();
        if (replicatesInfo.find(fromIp) == replicatesInfo.end()) {
            std::cout << "Known replicas: " << std::endl;
            for (const auto& replica: replicatesInfo) {
                mutex.lock();
                std::cout << replica.first << std::endl;
                mutex.unlock();
            }
            std::cout << "New one: " << fromIp << std::endl;
        }
        replicatesInfo[fromIp] = std::chrono::system_clock::now();
    }
}
