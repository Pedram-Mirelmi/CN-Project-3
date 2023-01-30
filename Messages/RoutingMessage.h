#pragma once
#include <memory>
#include "AbstractNetMessage.h"
using std::shared_ptr;


class Router;

class RoutingMessage : public AbstractNetMessage
{
    using string = std::string;
    shared_ptr<Router> m_sender;
    string m_destination;
    uint64_t m_cost;
public:
    RoutingMessage(shared_ptr<Router> sender, const string& dest, uint64_t cost);

    const shared_ptr<Router> &getSender() const;
    void setSender(const shared_ptr<Router> &newSender);
    uint64_t getCost() const;
    void setCost(uint64_t newCost);

    const string &getDestination() const;
    void setDestination(const string &newDestination);

    // AbstractNetMessage interface
public:
    MessageType getMessageType() override;
};
