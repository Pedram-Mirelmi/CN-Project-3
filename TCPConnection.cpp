#include "TCPConnection.h"
#include <tuple>
#include <cstring>
TCPConnection::TCPConnection(shared_ptr<Host>&& host, const string& endPoint)
    :m_endPoint(endPoint),
     m_correspondingHost(host)
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
    startTimeout(getEstimatedTimeout());
}

void TCPConnection::startSlowAgain()
{
    m_ssthresh = std::max(m_cwnd/2, (uint64_t)1);
    m_cwnd = 1;
    m_lastByteSent = m_lastPacketAcked;
}

void TCPConnection::timeoutBody(std::chrono::duration<long> duration)
{
    bool dummy;
    m_isTimeoutActive = true;
    if(!m_timeoutNotifier.wait_dequeue_timed(dummy, duration)) // timeout
    {
        std::scoped_lock<std::mutex> scopedLock(m_connectionLock);
        if(m_isTimeoutActive)
        {
            startSlowAgain();
            sendNextWindow();
            startTimeout(getEstimatedTimeout());
        }
    }
    m_isTimeoutActive = false;
}

void TCPConnection::startTimeout(std::chrono::duration<long> duration)
{
    m_timeoutThreads.push_back(std::thread(&TCPConnection::timeoutBody, this, std::move(duration)));
}

void TCPConnection::stopTimeout()
{
    m_isTimeoutActive = false;
    m_connectionLock.unlock();
    m_timeoutNotifier.enqueue(true); // anything
    m_timeoutNotifier = moodycamel::BlockingConcurrentQueue<bool>();
    for(auto& t : m_timeoutThreads)
        t.join();
    m_timeoutThreads.clear();
    m_connectionLock.lock();
}



void TCPConnection::handleAck(shared_ptr<Packet> packet)
{
    if(m_connectionLock.try_lock())
    {
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
        m_connectionLock.unlock();
    }
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

std::chrono::duration<long> TCPConnection::getEstimatedTimeout()
{

}

void TCPConnection::sendNextWindow()
{
    m_lastByteSent += m_cwnd;
    for(auto i = m_lastPacketAcked; i < m_lastPacketAcked + m_cwnd; i++)
    {
        m_correspondingHost->sendPacket(m_outBuffer[i]);
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

