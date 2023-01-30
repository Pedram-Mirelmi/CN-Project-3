#include "./AbstractNetMessage.h"


class HandShake : public AbstractNetMessage
{
    // AbstractNetMessage interface
public:
    MessageType getMessageType() override;
};
