#include <thread>
#include <iostream>
#include <uuid/uuid.h>
#include <unistd.h>
#include <random>
#include <algorithm>
#include <regex>
#include "TreeNode.h"

TreeNode::TreeNode(int port, std::string_view name, double loss, std::string_view ancestor, int ancestorPort)
        : TreeNode(port, name, loss) {

    this->ancestor = ancestor;
    this->ancestorPort = ancestorPort;

    if (!this->ancestor.empty()) {
        auto anc = MySocket(this->ancestorPort, this->ancestor);
        myAbonents[anc.ipaddrAndPort()] = anc;
    }


}

TreeNode::TreeNode(int port, std::string_view name, double loss) : port(port), name(name), procentOfLoss(loss),
                                                                   nodeSocket(port) {
    nodeSocket.open();
    nodeSocket.bind();
}

void TreeNode::workBody() {
    std::thread receiver(&TreeNode::receiving, this);
    receiver.detach();
    std::thread sender(&TreeNode::sending, this);
    sender.detach();
    std::thread fixer(&TreeNode::sendingLostMessages, this);
    fixer.join();
}


void TreeNode::receiving() {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dis(0, 100);

    while (1) {
        std::cout << "waiting for message!" << std::endl;
        auto msg = nodeSocket.read(otherSocket); //читаем сообщение
        auto rand = dis(mt);
        if (rand < procentOfLoss) {
            std::cerr << "wasted" << std::endl;
            continue; //потрачено
        }

        std::string recvUuid = msg.substr(msg.size() - UUID_SIZE_BUF, UUID_SIZE_BUF);
        auto sender = otherSocket.ipaddrAndPort();
        if (msg.find(SUCCESS_MARKER) != std::string::npos && msg.length() == SUCCESS_MARKER_LENGTH) { //дошел ответ
            mutex.lock();
            messagesStatus[recvUuid].second = true; //клиент живой
            mutex.unlock();
        } else {
            //сообщение еще не было получено
            if (messages.find(recvUuid) == messages.end()) {
                std::cout << std::regex_replace(msg, std::regex(recvUuid), "") << std::endl;
                std::unique_lock<std::mutex> lock(mutex);
                messages[recvUuid] = msg;
                myAbonents[sender] = otherSocket;
                messagesStatus[recvUuid] = std::pair(sender, true);
                cond_var.notify_one();
                nodeSocket.write(otherSocket, SUCCESS_MARKER + recvUuid);
            }
        }
    }
}

void TreeNode::sending() {
    while (1) {
        std::unique_lock<std::mutex> lock(mutex);
        while (myAbonents.empty()) {
            std::cout << "i'm root... wait" << std::endl;
            cond_var.wait(lock);
        }
        lock.unlock();

        std::cout << "enter message" << std::endl;
        std::string message;
        std::cin >> message;

        uuid_t uuid;
        uuid_generate_random(uuid);
        char uuidBuffer[UUID_SIZE_BUF];
        uuid_unparse(uuid, uuidBuffer);
        message += uuidBuffer;
        messages[uuidBuffer] = message;

        for (auto &abonent : myAbonents) {
            mutex.lock();
            messagesStatus[uuidBuffer].second = false;
            mutex.unlock();
            messagesStatus[uuidBuffer].first = abonent.first;
            nodeSocket.write(abonent.second, message);
        }
    }
}


void TreeNode::sendingLostMessages() {

    while (1) {
        for (const auto &status : messagesStatus) {
            mutex.lock();
            //если сообщение не дошло, то пересылаем его той же ноде
            if (!status.second.second) {
                //здесь можно несколько раз отправлять
                std::cerr << "vislal proveryay" << std::endl;
                nodeSocket.write(myAbonents[status.second.first], messages[status.first]);
            }
            mutex.unlock();
        }

        std::this_thread::sleep_for(std::chrono::seconds(TIME_OUT));

        for (auto it = messagesStatus.begin(); it != messagesStatus.end();) {
            mutex.lock();
            if (!it->second.second) {
                std::cerr << "Sdoh chelik: " + it->second.first << std::endl;
                myAbonents.erase(it->second.first);
                messages.erase(it->first);
                it = messagesStatus.erase(it);
            } else
                it++;
            mutex.unlock();
        }
    }
}
