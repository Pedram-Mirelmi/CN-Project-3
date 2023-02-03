#include <iostream>
#include <tuple>
#include <cstring>
#include "TCPConnection.h"
#include "./Nodes/Host.hpp"

#define print(x) std::cout << x << std::endl;

uint64_t TCPConnection::Packet_Size = 10;
uint16_t TCPConnection::maxPacketId = uint64_t((2<<15) - 1); // 2**15 - 1

TCPConnection::TCPConnection(shared_ptr<Host> host, const string& endPoint, duration initRTT)
    :m_endPoint(endPoint),
      m_correspondingHost(host),
      m_estimatedRTT(initRTT)
{

}

TCPConnection::TCPConnection(TCPConnection &&other)
{
    *(this) = std::move(other);
}

TCPConnection &TCPConnection::operator=(TCPConnection &&other)
{
    m_endPoint = std::move(other.m_endPoint);
    m_correspondingHost = std::move(other.m_correspondingHost);

    m_cwnd = other.m_cwnd;
    m_ssthresh = other.m_ssthresh;
    m_estimatedRTT = std::move(other.m_estimatedRTT);
    m_msgBuffer = std::move(other.m_msgBuffer);
    m_outBuffer = std::move(other.m_outBuffer);
    m_senderLastPacketAcked = other.m_senderLastPacketAcked;
    m_lastPacketSent = other.m_lastPacketSent;
    m_timeoutThreads = std::move(other.m_timeoutThreads);
    m_timeoutNotifier = std::move(other.m_timeoutNotifier);
    m_isTimeoutAllowed = other.m_isTimeoutAllowed;
    m_repeatDelay = other.m_repeatDelay;

    m_isOpen = other.m_isOpen;

    m_inBuffer = std::move(other.m_inBuffer);
    m_currentMessageSize = other.m_currentMessageSize;
    m_bytesReceivedSoFar = other.m_bytesReceivedSoFar;
    m_numberOfTotalPackets = other.m_numberOfTotalPackets;
    m_receiverLastPacketAcked = other.m_receiverLastPacketAcked;

    return *this;
}

void TCPConnection::takePacket(shared_ptr<Packet> packet)
{
    if(packet->isAck()) // this is sender
        handleAck(std::move(packet));
    else // it's data so this is reciever
        handleData(std::move(packet));
}

void TCPConnection::closeConnection()
{
    std::scoped_lock<std::mutex> scopedLock(m_connectionLock);
    m_isOpen = false;
    stopTimeout();
}

void TCPConnection::resetConnection()
{
    closeConnection();
    std::scoped_lock<std::mutex> scopedLock(m_connectionLock);
    auto msgBuffer = std::move(m_msgBuffer);
    *(this) = TCPConnection(m_correspondingHost, m_endPoint, m_estimatedRTT);
    m_msgBuffer = std::move(msgBuffer);
    m_isOpen = true;
}


void TCPConnection::sendMessage(const std::vector<char>& msgBuffer, uint64_t repeatDelay)
{
    resetConnection();
    m_repeatDelay = repeatDelay;
    m_msgBuffer = msgBuffer;
    packetize(msgBuffer);
    sendNextWindow();
    m_isTimeoutAllowed = true;
    startTimeout(getEstimatedTimeout());
}

void TCPConnection::sendMessage()
{
    auto repeateDelay = m_repeatDelay;
    resetConnection();
    m_repeatDelay = repeateDelay;

    sendNextWindow();

    m_isTimeoutAllowed = true;
    startTimeout(getEstimatedTimeout());
}

void TCPConnection::startSlowAgain()
{
    m_ssthresh = std::max(m_cwnd/2, (uint64_t)1);
    m_cwnd = 1;
    m_lastPacketSent = m_senderLastPacketAcked;
}

void TCPConnection::timeoutBody(duration duration)
{
    bool dummy;
    if(!m_timeoutNotifier.wait_dequeue_timed(dummy, duration)) // timeout
    {
        std::scoped_lock<std::mutex> scopedLock1(m_connectionLock);
        if(m_isTimeoutAllowed && m_isOpen)
        {
            startSlowAgain();
            sendNextWindow();
            startTimeout(getEstimatedTimeout());
            return;
        }
    }
}

void TCPConnection::startTimeout(duration duration)
{
    m_timeoutThreads.push_back(std::thread(&TCPConnection::timeoutBody, this, std::move(duration)));
}

void TCPConnection::stopTimeout()
{
    m_isTimeoutAllowed = false;
    m_timeoutNotifier.enqueue(true); // anything
    m_connectionLock.unlock();
    for(auto& t : m_timeoutThreads)
        if(t.joinable())
            t.join();
    m_timeoutThreads.clear();
    m_connectionLock.lock();
    m_timeoutNotifier = moodycamel::BlockingConcurrentQueue<bool>();
}


void TCPConnection::handleAck(shared_ptr<Packet> packet)
{
    if(m_isOpen)
    {
        std::scoped_lock<std::mutex> scopedLock(m_connectionLock);
        measureRTT(packet);
        if(packet->getPacketId() == (m_senderLastPacketAcked + 1)%maxPacketId) // OK!
        {
            m_senderLastPacketAcked++;
            m_outBuffer[m_senderLastPacketAcked].isAcked = true;
            if(packet->getPacketId() == m_lastPacketSent%maxPacketId) // perfect! Next window
            {
                if(m_senderLastPacketAcked == m_outBuffer.size() - 1)
                {
                    stopTimeout();
                    onDataSentCompletely();
                    return;
                }
                if(m_cwnd < m_ssthresh)
                    m_cwnd = std::min(m_cwnd<<1, (uint64_t)maxPacketId);
                else
                    m_cwnd++;
                stopTimeout();
                sendNextWindow();
                m_isTimeoutAllowed = true;
                startTimeout(getEstimatedTimeout());
            }
        }
        else if(packet->getPacketId() > (m_senderLastPacketAcked + 1)%maxPacketId) // lost packet
        {
            startSlowAgain();
            stopTimeout();
            sendNextWindow();
            m_isTimeoutAllowed = true;
            startTimeout(getEstimatedTimeout());
        }
    }
}

void TCPConnection::measureRTT(shared_ptr<Packet> packet)
{
    auto i = m_lastPacketSent;
    duration newRTT;
    while (i>0)
    {
        if(m_outBuffer[i--].packet->getPacketId() == packet->getPacketId())
            break;
    }
    newRTT = std::chrono::high_resolution_clock::now() - m_outBuffer[i+1].timeSent;
    m_estimatedRTT = duration((uint64_t)(ALPHA*m_estimatedRTT.count() + BETA*newRTT.count()));
}

void TCPConnection::packetize(const std::vector<char> &wholeMessage)
{
    uint64_t size = wholeMessage.size();
    auto numberOfPacketsNeeded = size/Packet_Size + 1; // +1 (first packet)
    if(size%Packet_Size)
        numberOfPacketsNeeded++;
    m_outBuffer.reserve(numberOfPacketsNeeded + 1); // first element is dummy
    m_outBuffer.push_back({});

    auto msgBuff = wholeMessage.data();

    auto firstPacketBody = std::vector<char>(8);
    *((uint64_t*)firstPacketBody.data()) = size;

    auto firstPacket = std::make_shared<Packet>(false, 1, firstPacketBody, m_correspondingHost->getAddr(), m_endPoint);

    m_outBuffer.push_back({firstPacket, false, false, {}});
    auto capacity = m_outBuffer.capacity();
    for(uint64_t i = 2; i < capacity; i++)
    {
        auto packet = make_shared<Packet>(
                    false,
                    i%maxPacketId,
                    std::vector<char>(msgBuff+(i-2)*Packet_Size, msgBuff+(i-2+1)*Packet_Size),
                    m_correspondingHost->getAddr(),
                    m_endPoint);

        OutBufferElement element{std::move(packet), false, false, {}};
        m_outBuffer.push_back(std::move(element));
    }
    if(size%Packet_Size)
        m_outBuffer.back().packet->getBody().resize(size%Packet_Size);
}

TCPConnection::duration TCPConnection::getEstimatedTimeout()
{
    return m_estimatedRTT;
}


const TCPConnection::string &TCPConnection::getEndPoint() const
{
    return m_endPoint;
}


void TCPConnection::sendNextWindow()
{
//    print("sending window")
    m_lastPacketSent = m_senderLastPacketAcked + m_cwnd;
    m_lastPacketSent = std::min(m_lastPacketSent, m_outBuffer.size()-1);
    auto startingPoint = m_senderLastPacketAcked + 1;
    for(auto i = startingPoint; i <= m_lastPacketSent; i++)
    {
//        std::cout << "sending " << m_outBuffer[i].packet->getPacketId() << std::endl;
        m_correspondingHost->sendPacket(m_outBuffer[i].packet);
        if(!m_outBuffer[i].isSent)
        {
            m_outBuffer[i].isSent = true;
            m_outBuffer[i].timeSent = std::chrono::high_resolution_clock::now();
        }
    }
}

void TCPConnection::onDataSentCompletely()
{
    stopTimeout();
}

void TCPConnection::handleData(shared_ptr<Packet> packet)
{
//    print("handling data");
    auto& body = packet->getBody();
    if(!m_inBuffer.size()) // empty buffer: first packet
    {
        m_currentMessageSize = *((uint64_t*)body.data());
        m_numberOfTotalPackets = (m_currentMessageSize/Packet_Size) + (m_currentMessageSize%Packet_Size ? 1 : 0) + 1;
        m_inBuffer.push_back(packet);
    }
    else
    {
        if(packet->getPacketId() == (m_receiverLastPacketAcked%maxPacketId)+1) //OK
        {
            m_inBuffer.push_back(packet);
            m_bytesReceivedSoFar += packet->getBody().size();
        }
    }
    if(m_currentMessageSize == m_bytesReceivedSoFar)
        onNewFileCompletelyReceived();

    if(packet->getPacketId() > (m_receiverLastPacketAcked%maxPacketId))
    {
        m_receiverLastPacketAcked++;
        sendPacketAck(std::move(packet));
    }
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
    print("finished:")
    for(auto& packet : m_inBuffer)
        print(packet->getBody().data());
    m_inBuffer.clear();
    m_bytesReceivedSoFar = m_currentMessageSize = 0;
}

