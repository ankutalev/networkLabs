#include "ReplicateCounter.h"


ReplicateCounter::ReplicateCounter(std::string_view mltcAddr) {

}

void ReplicateCounter::aliveChecker() {
    std::this_thread::sleep_for(std::chrono::seconds(ALIVE_DURATION));
    auto currentTime = std::chrono::system_clock::now();
    for (auto it = replicatesInfo.begin(); it != replicatesInfo.end();) {
        mutex.lock();
        auto lastTime = it->second;
        if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastTime).count() < ALIVE_DURATION) {
            it = replicatesInfo.erase(it);
        }
        else
            ++it;
        mutex.unlock();
    }
}
