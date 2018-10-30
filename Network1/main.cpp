#include <iostream>
#include "ReplicateCounter.h"

int main(int argc, char* argv[]) {

    std::string mltcstAddr = argc < 2 ? "224.0.1.15" : argv[1];
    ReplicateCounter counter(mltcstAddr);
    counter.startWorking();
    return 0;

}

