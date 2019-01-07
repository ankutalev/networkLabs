#include <iostream>
#include <signal.h>
#include "SocksProxy.h"

int main(int argc, char* argv[]) {
    SocksProxy* proxy;
    if (argc < 2) {
        proxy = new SocksProxy;
    } else {
        int port = std::stoi(argv[1]);
        proxy = new SocksProxy(port);
    }
    proxy->startWorking();
    delete proxy;
    return 0;
}
