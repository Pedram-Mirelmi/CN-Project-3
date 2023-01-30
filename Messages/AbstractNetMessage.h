#pragma once
#include <memory>
class AbstractNetMessage : public std::enable_shared_from_this<AbstractNetMessage>
{
public:
    enum MessageType {ROUTING_MESSAGE, PACKET, HAND_SHAKE};
    virtual MessageType getMessageType() = 0;
};
