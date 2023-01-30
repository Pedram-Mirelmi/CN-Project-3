#pragma once
#include <vector>
#include "AbstractNetMessage.h"

class Packet : public AbstractNetMessage
{
    bool m_isAck;
    uint16_t m_packetId;
    std::string m_source;
    std::string m_destination;
    std::vector<char> m_body;
public:
    Packet() = default;
    Packet(bool isAck, uint16_t id, const std::vector<char>& body, const std::string& source, const std::string& destination);
    Packet(bool isAck, uint16_t id, std::vector<char>&& body, const std::string& source, const std::string& destination);

    const std::vector<char> &getBody() const;
    void setBody(const std::vector<char> &newBody);
    void setBody(std::vector<char>&& newBody);

    const std::string &getSource() const;
    void setSource(const std::string &newSource);

    const std::string &getDestination() const;
    void setDestination(const std::string &newDestination);



    // AbstractNetMessage interface
public:
    MessageType getMessageType() override;
    bool isAck() const;
    void setIsAck(bool newIsAck);
    uint16_t packetId() const;
    void setPacketId(uint16_t newPacketId);
};
