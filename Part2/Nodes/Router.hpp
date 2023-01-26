#pragma once
#include <string>
#include <thread>

#include "AbstractNode.hpp"
#include "../Messages/RoutingMessage.h"
#include "../Messages/Packet.h"

using std::make_shared;

class Router : public AbstractNode
{
    RouterLinkMap_t m_toRoutersLinks;
    HostLinkMap_t m_toHostsLinks;
public:
    Router(const string &addr);
    // AbstractNode interface
public:
    void startNode() override;
    void addToRouterLink(shared_ptr<Router> router, uint64_t cost) override;
    void addToHostLink(shared_ptr<Host> host, uint64_t cost) override;
    void updateToRouteLink(shared_ptr<Router> router, uint64_t cost) override;
    void updateToHostLink(shared_ptr<Host> host, uint64_t cost) override;
    void takeMessage(shared_ptr<AbstractNetMessage> message) override;
    NodeType getType() override;
private:
    void handleNewMessage(shared_ptr<AbstractNetMessage> message);
    void updateRoutingTable(shared_ptr<RoutingMessage> message);
    void broadCastNewLink(string destination, uint64_t updatedCost);
    void routeAndForwardPacket(shared_ptr<Packet> packet);

};


