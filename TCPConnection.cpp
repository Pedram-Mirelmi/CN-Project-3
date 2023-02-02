#include "TCPConnection.h"
#include "./Nodes/Host.hpp"
#include <tuple>
#include <cstring>

uint64_t TCPConnection::Packet_Size = 10;

TCPConnection::TCPConnection(shared_ptr<Host>&& host, const string& endPoint, duration initRTT)
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
    m_cwnd = other.m_cwnd;
    m_ssthresh = other.m_ssthresh;

    m_inBuffer = std::move(other.m_inBuffer);
    m_outBuffer = std::move(other.m_outBuffer);
    m_lastByteSent = other.m_lastByteSent;
    m_lastPacketAcked = other.m_lastPacketAcked;
    m_timeoutThreads = std::move(other.m_timeoutThreads);
    m_timeoutNotifier = std::move(other.m_timeoutNotifier);

    m_correspondingHost = std::move(other.m_correspondingHost);

    m_currentMessageSize = other.m_currentMessageSize;
    m_bytesReceivedSoFar = other.m_bytesReceivedSoFar;
    m_isTimeoutActive = other.m_isTimeoutActive;
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
    m_lastPacketAcked = 0;
    m_lastByteSent = 0;
    m_cwnd = 1;
    sendNextWindow();
    startTimeout(getEstimatedTimeout());
}

void TCPConnection::startSlowAgain()
{
    m_ssthresh = std::max(m_cwnd/2, (uint64_t)1);
    m_cwnd = 1;
    m_lastByteSent = m_lastPacketAcked;
}

void TCPConnection::timeoutBody(duration duration)
{
    bool dummy;
    m_isTimeoutActive = true;
    if(!m_timeoutNotifier.wait_dequeue_timed(dummy, duration)) // timeout
    {
        std::scoped_lock<std::mutex> scopedLock(m_timeoutLock);
        if(m_isTimeoutActive)
        {
            startSlowAgain();
            sendNextWindow();
            startTimeout(getEstimatedTimeout());
        }
        m_isTimeoutActive = false;
    }
    m_isTimeoutActive = false;
}

void TCPConnection::startTimeout(duration duration)
{
    m_timeoutThreads.push_back(std::thread(&TCPConnection::timeoutBody, this, std::move(duration)));
}

void TCPConnection::stopTimeout()
{
    m_isTimeoutActive = false;
    m_timeoutLock.unlock();
    m_timeoutNotifier.enqueue(true); // anything
    m_timeoutNotifier = moodycamel::BlockingConcurrentQueue<bool>();
    for(auto& t : m_timeoutThreads)
        t.join();
    m_timeoutThreads.clear();
    m_timeoutLock.lock();
}



void TCPConnection::handleAck(shared_ptr<Packet> packet)
{
    m_timeoutLock.lock();
    if(m_isTimeoutActive)
    {
        measureRTT(packet);
        if(packet->getPacketId() == m_lastPacketAcked + 1) // OK!
        {
            m_lastPacketAcked++;
            if(packet->getPacketId() == m_lastByteSent) // perfect! Next window
            {
                if(m_cwnd < m_ssthresh)
                    m_cwnd = m_cwnd<<1;
                else
                    m_cwnd++;
                stopTimeout();
                sendNextWindow();
                startTimeout(getEstimatedTimeout());
            }
        }
        else // lost packet
        {
            startSlowAgain();
            stopTimeout();
            sendNextWindow();
            startTimeout(getEstimatedTimeout());
        }
    }

    m_timeoutLock.unlock();
}

void TCPConnection::measureRTT(shared_ptr<Packet> packet)
{
    auto i = m_lastByteSent;
    duration newRTT;
    bool found;
    while (i>=0)
    {
        if(m_outBuffer[--i].packet->getPacketId()==packet->getPacketId())
            found = true;
    }
    if(!found)
    {
        while (i>=0)
        {
            if(m_outBuffer[--i].packet->getPacketId()==packet->getPacketId())
                break;
        }
    }
    newRTT = std::chrono::high_resolution_clock::now() - m_outBuffer[i+1].timeSent;
    m_estimatedRTT = duration((uint64_t)(ALPHA*m_estimatedRTT.count() + BETA*newRTT.count()));
}

void TCPConnection::packetize(const std::vector<char> &wholeMessage)
{
    uint64_t size = wholeMessage.size();
    auto numberOfPacketsNeeded = size/10 + 1; // +1 (first packet)
    if(size%10)
        numberOfPacketsNeeded++;
    m_outBuffer.reserve(numberOfPacketsNeeded + 1); // first element is dummy
    m_outBuffer.push_back({});

    auto msgBuff = wholeMessage.data();

    auto firstPacketBody = std::vector<char>(8);
    memcpy(firstPacketBody.data(), (char*)size, 8);

    auto firstPacket = std::make_shared<Packet>(false, 0, firstPacketBody, m_correspondingHost->getAddr(), m_endPoint);

    m_outBuffer.push_back({firstPacket, false, false, {}});
    for(uint64_t i = 1; i < m_outBuffer.capacity(); i++)
    {
        auto packet = make_shared<Packet>(
                    false,
                    i,
                    std::vector<char>(msgBuff+(i+1)*10, msgBuff+(i+2)*10),
                    m_correspondingHost->getAddr(),
                    m_endPoint);

        OutBufferElement element{std::move(packet), false, false, {}};
        m_outBuffer.push_back(std::move(element));
    }
}

TCPConnection::duration TCPConnection::getEstimatedTimeout()
{
    return m_estimatedRTT;
}

void TCPConnection::sendNextWindow()
{
    m_lastByteSent += m_cwnd;
    for(auto i = m_lastPacketAcked; i < m_lastPacketAcked + m_cwnd; i++)
    {
        m_correspondingHost->sendPacket(m_outBuffer[i].packet);
        if(!m_outBuffer[i].isSent)
        {
            m_outBuffer[i].isSent = true;
            m_outBuffer[i].timeSent = std::chrono::high_resolution_clock::now();
        }
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

