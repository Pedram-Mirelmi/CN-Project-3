#pragma once
#include <string>
#include <thread>
#include <unordered_map>
#include <memory>
#include "AbstractNode.hpp"
#include "../Messages/RoutingMessage.h"
#include "../Messages/Packet.h"
#include "../DVAlgController.h"
class Host : public AbstractNode
{
    shared_ptr<DVAlgController> m_algController;
public:
    Host(const string &addr);
    Host(const Host& other) = delete;
    Host& operator=(const Host& other) = delete;


    // AbstractNode interface
public:
    void startNode() override;
    void takeMessage(shared_ptr<AbstractNetMessage> message) override;
    NodeType getType() override;
private:
    void handleNewMessage(shared_ptr<AbstractNetMessage> message);
    void handlePacket(shared_ptr<Packet> packet);
};


