#include "Packet.h"

Packet::Packet(bool isAck, const std::vector<char>& body, const std::string& source, const std::string& destination)
    : m_isAck(isAck), m_body(body), m_source(source), m_destination(destination)
{

}

Packet::Packet(bool isAck, std::vector<char> &&body, const std::string& source, const std::string& destination)
    : m_isAck(isAck), m_body(std::move(body)), m_source(source), m_destination(destination)
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

AbstractNetMessage::MessageType Packet::getMessageType()
{
    return MessageType::PACKET;
}
