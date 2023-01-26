#include <iostream>
#include "Host.hpp"
#include "Router.hpp"


Host::Host(const string &addr)
    : AbstractNode(addr), m_algController(DVAlgController::getInstance())
{

}

void Host::startNode()
{
    m_thread = std::thread([this](){
//        this->isRunning
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
            auto betweenNodeCost = m_links[routingMessage->getSender()->getAddr()].cost;
            updateRoutingTable(routingMessage->getDestination(), routingMessage->getCost()+betweenNodeCost, routingMessage->getSender());
            m_algController->dec();
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
    std::cout << "A packet with body size of " << packet->getBody().size() << " received" << std::endl;
    // TODO
}

void Host::takeMessage(shared_ptr<AbstractNetMessage> message)
{
    m_nodeQueue.enqueue(std::move(message));
}

AbstractNode::NodeType Host::getType()
{
    return NodeType::HOST;
}

