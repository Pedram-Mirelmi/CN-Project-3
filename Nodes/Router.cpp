#include <iostream>
#include "Router.hpp"
#include "Host.hpp"
Router::Router(const string &addr) :
    AbstractNode(addr), m_algController(NetworkController::getInstance())
{

}

void Router::startNode()
{
    m_thread = std::thread([this](){
        m_isRunning = true;
        m_algController->incRunningNodeCounter();
        for(auto& pair : m_routingTable)
        {
            broadCastNewLink(pair.first, pair.second.cost);
        }
        while (true)
        {
            shared_ptr<AbstractNetMessage> message;
            this->m_nodeQueue.wait_dequeue(message);
            if(m_mustStop)
                break;
            this->handleNewMessage(std::move(message));
        }
        m_mustStop = false;
        m_isRunning = false;
        m_algController->decConvergeCounter();
    });
}



void Router::handleNewMessage(shared_ptr<AbstractNetMessage> message)
{
    switch (message->getMessageType())
    {
    case AbstractNetMessage::ROUTING_MESSAGE:
    {
        auto routingMessage = std::dynamic_pointer_cast<RoutingMessage>(message);
        auto betweenNodeCost = m_links[routingMessage->getSender()->getAddr()].cost;
        updateRoutingTable(routingMessage->getDestination(), routingMessage->getCost()+betweenNodeCost, routingMessage->getSender());
        m_algController->decConvergeCounter();
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

bool Router::updateRoutingTable(const string& dest, uint64_t cost, shared_ptr<AbstractNode> nextHop)
{
    if(AbstractNode::updateRoutingTable(dest, cost, std::move(nextHop)))
    {
        broadCastNewLink(dest, cost);
        return true;
    }
    return false;
}

void Router::broadCastNewLink(string destination, uint64_t updatedCost)
{
    if(true)
    {

        for(auto& pair : m_toHostsLinks)
        {
            pair.second.host->takeMessage(make_shared<RoutingMessage>(std::dynamic_pointer_cast<Router>(shared_from_this()), destination, updatedCost));
            m_algController->incConvergeCounter();
        }
        for(auto& pair : m_toRoutersLinks)
        {
            pair.second.router->takeMessage(make_shared<RoutingMessage>(std::dynamic_pointer_cast<Router>(shared_from_this()), destination, updatedCost));
            m_algController->incConvergeCounter();
        }
    }
}

void Router::routeAndForwardPacket(shared_ptr<Packet> packet)
{
    auto destAddr = packet->getDestination();
    if(!m_routingTable.contains(destAddr))
    {
        std::cout << "Address not found" << std::endl;
        return;
    }
    m_routingTable[destAddr].nextHop->takeMessage(std::move(packet));
}


AbstractNode::NodeType Router::getType()
{
    return NodeType::ROUTER;
}

