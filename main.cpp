#include <iostream>
#include "TreeNode.h"

int main(int argc, char *argv[]) {

    auto port = std::stoi(argv[1]);
    std::string name = argv[2];
    auto loss = std::stod(argv[3]);
    TreeNode *node;
    if (argc == 4) {
        node = new TreeNode(port, name, loss);
    } else {
        std::string ancestorIp = argv[4];
        auto ancPort = std::stoi(argv[5]);
        node = new TreeNode(port, name, loss, ancestorIp, ancPort);
    }
    node->workBody();
    delete node;

    return 0;
}