#pragma once
#include <vector>
#include "AbstractNetMessage.h"

class Packet : public AbstractNetMessage
{
    bool m_isAck;
    std::vector<char> m_body;
    std::string m_source, m_destination;
public:
    Packet() = default;
    Packet(bool isAck, const std::vector<char>& body, const std::string& source, const std::string& destination);
    Packet(bool isAck, std::vector<char>&& body, const std::string& source, const std::string& destination);

    const std::vector<char> &getBody() const;
    void setBody(const std::vector<char> &newBody);
    void setBody(std::vector<char>&& newBody);


    // AbstractNetMessage interface
public:
    MessageType getMessageType() override;
    const std::string &getSource() const;
    void setSource(const std::string &newSource);
    const std::string &getDestination() const;
    void setDestination(const std::string &newDestination);
};
