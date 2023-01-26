#include "RoutingMessage.h"


RoutingMessage::RoutingMessage(shared_ptr<Router> sender, const string& dest, uint64_t cost)
    : m_sender(std::move(sender)), m_destination(dest), m_cost(cost)
{}

const shared_ptr<Router> &RoutingMessage::getSender() const
{
    return m_sender;
}

void RoutingMessage::setSender(const shared_ptr<Router> &newSender)
{
    m_sender = newSender;
}

const std::string &RoutingMessage::getDestination() const
{
    return m_destination;
}

void RoutingMessage::setDestination(const string &newDestination)
{
    m_destination = newDestination;
}

uint64_t RoutingMessage::getCost() const
{
    return m_cost;
}

void RoutingMessage::setCost(uint64_t newCost)
{
    m_cost = newCost;
}


AbstractNetMessage::MessageType RoutingMessage::getMessageType()
{
    return MessageType::ROUTING_MESSAGE;
}
