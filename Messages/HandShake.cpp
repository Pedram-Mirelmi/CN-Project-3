#include "HandShake.h"

AbstractNetMessage::MessageType HandShake::getMessageType()
{
    return MessageType::HAND_SHAKE;
}
