#include <iostream>
#include "Host.hpp"
#include "Router.hpp"


Host::Host(const string &addr)
    : AbstractNode(addr), m_algController(NetworkController::getInstance())
{

}

void Host::startNode()
{
    m_thread = std::thread([this](){
        this->m_isRunning = true;
        m_algController->incRunningNodeCounter();
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
        m_algController->decRunningNodeCounter();
    });
}

void Host::handleNewMessage(shared_ptr<AbstractNetMessage> message)
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
            auto packet = std::dynamic_pointer_cast<Packet>(std::move(message));
            handlePacket(std::move(packet));
            break;
        }
        default:
        {
            std::cout << "unknown type of message received!" << std::endl;
        }

    }
}


void Host::handlePacket(shared_ptr<Packet> packet)
{

}

AbstractNode::NodeType Host::getType()
{
    return NodeType::HOST;
}

