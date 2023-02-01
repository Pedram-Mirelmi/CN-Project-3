#include "TCPConnection.h"
#include <cstring>
TCPConnection::TCPConnection(shared_ptr<Host>&& host, const string& endPoint)
    :m_correspondingHost(host),
     m_endPoint(endPoint)
{

}

bool TCPConnection::takePacket(shared_ptr<Packet> packet)
{
    if(packet->isAck()) // this is sender
        handleAck(std::move(packet));
    else // it's data so this is reciever
        handleData(std::move(packet));
}

void TCPConnection::sendMessage(const std::vector<char> &msgBuffer)
{
    packetize(msgBuffer);
    m_lastPacketAcked = -1;
    m_lastByteSent = 0;
    m_correspondingHost->sendPacket(m_outBuffer[0]);
}


void TCPConnection::handleAck(shared_ptr<Packet> packet)
{
    if(packet->get)
}

void TCPConnection::packetize(const std::vector<char> &wholeMessage)
{
    uint64_t size = wholeMessage.size();
    auto numberOfPacketsNeeded = size/10 + 1; // +1 (first packet)
    if((size)%10)
        numberOfPacketsNeeded++;
    m_outBuffer.reserve(numberOfPacketsNeeded);

    auto msgBuff = wholeMessage.data();

    auto firstPacketBody = std::vector<char>(8);
    memcpy(firstPacketBody.data(), (char*)size, 8);

    m_outBuffer.push_back(std::make_shared<Packet>(false, 0, firstPacketBody, m_correspondingHost->getAddr(), m_endPoint));
    for(size_t i = 1; i < m_outBuffer.capacity(); i++)
    {
        m_outBuffer.push_back(make_shared<Packet>(
                        false,
                        i,
                        std::vector<char>(msgBuff+(i+1)*10, msgBuff+(i+2)*10),
                        m_correspondingHost->getAddr(),
                        m_endPoint
                ));
    }
}

void TCPConnection::handleData(shared_ptr<Packet> packet)
{
    auto& body = packet->getBody();
    if(!m_inBuffer.size()) // empty buffer: first packet
    {
        m_currentMessageSize = *((uint64_t*)body.data());
        m_bytesReceivedSoFar = body.size() - sizeof(uint64_t);
    }
    else
    {
        m_bytesReceivedSoFar += body.size();
    }
    m_inBuffer.push_back(packet);
    if(m_currentMessageSize == m_bytesReceivedSoFar)
        onNewFileCompletelyReceived();

    sendPacketAck(std::move(packet));
}

void TCPConnection::sendPacketAck(shared_ptr<Packet> packet)
{
    auto ack = make_shared<Packet>(*packet); // copy
    ack->setIsAck(true);
    ack->switchAddresses();
    m_correspondingHost->sendPacket(std::move(ack));
}

void TCPConnection::onNewFileCompletelyReceived()
{
    // TODO connect all packet bodies and log or save to a file
    m_inBuffer.clear();
    m_bytesReceivedSoFar = m_currentMessageSize = 0;
}

