#include <iostream>
#include "Router.hpp"
#include "Host.hpp"
Router::Router(const string &addr) :
    AbstractNode(addr)
{

}

void Router::startNode()
{
    m_thread = std::thread([this](){
        for(auto& pair : m_routingTable)
        {
            broadCastNewLink(pair.first, pair.second.cost);
        }
        while (true)
        {
            shared_ptr<AbstractNetMessage> message;
            this->m_nodeQueue.wait_dequeue(message);
            this->handleNewMessage(std::move(message));
        }
    });
}

void Router::addToRouterLink(shared_ptr<Router> router, uint64_t cost)
{
    m_toRoutersLinks[router->getAddr()] = {router, cost};
    broadCastNewLink(router->getAddr(), cost);
}

void Router::addToHostLink(shared_ptr<Host> host, uint64_t cost)
{
    m_toHostsLinks[host->getAddr()] = {host, cost};
    if(m_routingTable.contains(host->getAddr()) &&
       m_routingTable[host->getAddr()].cost > cost)
    {
        m_routingTable[host->getAddr()] = {host->getAddr(), cost, host};
        broadCastNewLink(host->getAddr(), cost);

    }
}

void Router::updateToRouteLink(shared_ptr<Router> router, uint64_t cost)
{
    addToRouterLink(router, cost);
}

void Router::updateToHostLink(shared_ptr<Host> host, uint64_t cost)
{
    addToHostLink(host, cost);
}

void Router::handleNewMessage(shared_ptr<AbstractNetMessage> message)
{
    switch (message->getMessageType())
    {
        case AbstractNetMessage::ROUTING_MESSAGE:
        {
            auto routingMessage = std::dynamic_pointer_cast<RoutingMessage>(message);
            updateRoutingTable(std::move(routingMessage));
            break;
        }
        case AbstractNetMessage::PACKET:
        {
            auto packet = std::dynamic_pointer_cast<Packet>(message);
            routeAndForwardPacket(packet);
            break;
        }
        default:
        {
            std::cout << "Unknown type of message received!" << std::endl;
        }
    }
}

void Router::updateRoutingTable(shared_ptr<RoutingMessage> message)
{
    auto destAddr = message->getDestination();
    auto senderAddr = message->getSender()->getAddr();
    auto newCostUntilSender = message->getCost();
    if(m_routingTable.contains(destAddr) &&
       m_routingTable[destAddr].cost > m_toRoutersLinks[senderAddr].cost + newCostUntilSender)
    { // update should take place
        m_routingTable[destAddr] = {message->getDestination(),
                                    m_toRoutersLinks[senderAddr].cost + message->getCost(),
                                    message->getSender()};
        broadCastNewLink(destAddr, m_routingTable[destAddr].cost);
    }

}

void Router::broadCastNewLink(string destination, uint64_t updatedCost)
{
    for(auto& pair : m_toHostsLinks)
    {
        pair.second.host->takeMessage(make_shared<RoutingMessage>(shared_ptr<Router>(this), destination, updatedCost));
    }
    for(auto& pair : m_toRoutersLinks)
    {
        pair.second.router->takeMessage(make_shared<RoutingMessage>(shared_ptr<Router>(this), destination, updatedCost));
    }
}

void Router::routeAndForwardPacket(shared_ptr<Packet> packet)
{
    auto destAddr = packet->getDestination();
    if(!m_routingTable.contains(destAddr))
    {

    }
}

void Router::takeMessage(shared_ptr<AbstractNetMessage> message)
{

}

AbstractNode::NodeType Router::getType()
{
    return NodeType::ROUTER;
}


