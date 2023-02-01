#include "Packet.h"

Packet::Packet(bool isAck, uint16_t id, const std::vector<char>& body, const std::string& source, const std::string& destination)
    : m_isAck(isAck),
      m_packetId(id),
      m_source(source),
      m_destination(destination),
      m_body(body)
{

}

Packet::Packet(bool isAck, uint16_t id, std::vector<char> &&body, const std::string& source, const std::string& destination)
    : m_isAck(isAck),
      m_packetId(id),
      m_source(source),
      m_destination(destination),
      m_body(std::move(body))
{

}

const std::string &Packet::getSource() const
{
    return m_source;
}

void Packet::setSource(const std::string &newSource)
{
    m_source = newSource;
}

const std::string &Packet::getDestination() const
{
    return m_destination;
}

void Packet::setDestination(const std::string &newDestination)
{
    m_destination = newDestination;
}

void Packet::switchAddresses()
{
    std::swap(m_source, m_destination);
}

const std::vector<char> &Packet::getBody() const
{
    return m_body;
}

void Packet::setBody(const std::vector<char> &newBody)
{
    m_body = newBody;
}

void Packet::setBody(std::vector<char> &&newBody)
{
    m_body = std::move(newBody);
}

bool Packet::isAck() const
{
    return m_isAck;
}

void Packet::setIsAck(bool newIsAck)
{
    m_isAck = newIsAck;
}

uint16_t Packet::getPacketId() const
{
    return m_packetId;
}

void Packet::setPacketId(uint16_t newPacketId)
{
    m_packetId = newPacketId;
}

AbstractNetMessage::MessageType Packet::getMessageType()
{
    return MessageType::PACKET;
}
