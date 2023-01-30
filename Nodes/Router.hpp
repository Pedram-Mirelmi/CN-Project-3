#pragma once
#include <string>
#include <thread>
#include <iostream>
#include "AbstractNode.hpp"
#include "../Messages/RoutingMessage.h"
#include "../Messages/Packet.h"
#include "../NetworkController.h"


using std::make_shared;

class Router : public AbstractNode
{
    shared_ptr<NetworkController> m_algController;
public:
    Router(const string &addr);
    Router(const Router& other) = delete;
    Router& operator=(const Router& other) = delete;

    // AbstractNode interface
public:
    void startNode() override;
    bool updateRoutingTable(const string& dest, uint64_t cost, shared_ptr<AbstractNode> nextHop) override;
    NodeType getType() override;
private:
    void handleNewMessage(shared_ptr<AbstractNetMessage> message);
    void broadCastNewLink(string destination, uint64_t updatedCost);
    void routeAndForwardPacket(shared_ptr<Packet> packet);

};


