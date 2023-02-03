#pragma once
#include <string>
#include <thread>
#include <unordered_map>
#include <memory>
#include "AbstractNode.hpp"
#include "../Messages/RoutingMessage.h"
#include "../Messages/Packet.h"
#include "../NetworkController.h"
#include "../TCPConnection.h"

class Host : public AbstractNode
{
    shared_ptr<NetworkController> m_algController;
    std::unordered_map<string, TCPConnection> m_connections;
public:
    using duration = std::chrono::high_resolution_clock::duration;
    using time_point = std::chrono::high_resolution_clock::time_point;
    static duration initRTT;

    string m_logFilename;
public:
    Host(const string &addr);
    Host(const Host& other) = delete;
    Host& operator=(const Host& other) = delete;

    void addTcpConnection(const string& endPoint, duration initRTT);
    void sendPacket(shared_ptr<Packet> packet);
    void sendMessageTo(const string& receiver, const std::vector<char>& data, uint64_t repeateDelay);
    // AbstractNode interface
public:
    void log(const string &msg) override;
    void startNode() override;
    void stopNode() override;
    NodeType getType() override;
    void handleNewMessage(shared_ptr<AbstractNetMessage> message) override;
private:
    void handlePacket(shared_ptr<Packet> packet);

};


