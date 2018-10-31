#include <iostream>
#include "ReplicateCounter.h"

int main(int argc, char* argv[]) {

    std::string mltcstAddr = (argc < 2) ? "224.0.1.15" : argv[1];
    std::string localhost = (argc < 2) ? "127.0.0.1" : "::1";
    ReplicateCounter counter(localhost, mltcstAddr);
    counter.startWorking();
    return 0;

}

