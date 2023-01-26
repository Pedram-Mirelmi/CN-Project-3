#include <iostream>
#include "Host.hpp"
#include "Router.hpp"


Host::Host(const string &addr)
    : AbstractNode(addr)
{

}

void Host::startNode()
{
    m_thread = std::thread([this](){
        while (true) {
            shared_ptr<AbstractNetMessage> message;
            this->m_nodeQueue.wait_dequeue(message);
            this->handleNewMessage(std::move(message));
        }
    });
}

void Host::handleNewMessage(shared_ptr<AbstractNetMessage> message)
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
            std::cout << "Host received routing message! " << std::endl;
        }
        default:
        {
            std::cout << "unknown type of message received!" << std::endl;
        }

    }
}

void Host::updateRoutingTable(shared_ptr<RoutingMessage> message)
{
    auto destAddr = message->getDestination();
    auto senderAddr = message->getSender()->getAddr();
    auto newCostUntilSender = message->getCost();
    if(m_routingTable.contains(destAddr) &&
       m_routingTable[destAddr].cost > m_toRoutersLinks[senderAddr].cost + newCostUntilSender)
    {
        m_routingTable[destAddr] = {message->getDestination(),
                                    m_toRoutersLinks[senderAddr].cost + message->getCost(),
                                    message->getSender()};
    }

}



void Host::addToRouterLink(shared_ptr<Router> router, uint64_t cost)
{
    m_toRoutersLinks[router->getAddr()] = {router, cost};
}

void Host::addToHostLink(shared_ptr<Host> host, uint64_t cost)
{
    std::cout << "Cannot connect a Host to another host! " << std::endl;
}

void Host::updateToRouteLink(shared_ptr<Router> router, uint64_t cost)
{
    if(m_toRoutersLinks.contains(router->getAddr()))
    {
        m_toRoutersLinks[router->getAddr()] = {router, cost};
    }
}

void Host::updateToHostLink(shared_ptr<Host> host, uint64_t cost)
{
    std::cout << "Cannot connect a Host to another host! " << std::endl;
}

void Host::takeMessage(shared_ptr<AbstractNetMessage> message)
{
    m_nodeQueue.enqueue(std::move(message));
}

AbstractNode::NodeType Host::getType()
{
    return NodeType::HOST;
}

