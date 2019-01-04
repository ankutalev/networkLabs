#include <iostream>
#include "SocksProxy.h"

int main(int argc, char* argv[]) {
    CacheProxy* proxy;
    if (argc < 2) {
        proxy = new CacheProxy;
    } else {
        int port = std::stoi(argv[1]);
        proxy = new CacheProxy(port);
    }
    proxy->startWorking();
    delete proxy;

    return 0;

}
//4 87
//5 -16
//6 -127
//7 71



//8 1
//9 -69
//449
// 000000011

//1 11000001