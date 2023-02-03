#include <iostream>
#include <tuple>
#include <cstring>
#include <fstream>
#include "TCPConnection.h"
#include "./Nodes/Host.hpp"

#define print(x) std::cout << x << std::endl;

uint64_t TCPConnection::Packet_Size = 10;
uint16_t TCPConnection::maxPacketId = uint64_t((2<<15) - 1); // 2**15 - 1

void TCPConnection::log(const string &msg)
{
    std::scoped_lock<std::mutex> scopedLock(m_logLock);
    std::ofstream logFile(m_logFilename, std::ios_base::app);
    logFile << msg << '\n';
    logFile.close();
}

TCPConnection::TCPConnection(shared_ptr<Host> host, const string& endPoint, duration initRTT)
    : m_endPoint(endPoint),
      m_correspondingHost(host),
      m_estimatedRTT(initRTT)
{
    m_logFilename = "./log/TCP: " + m_correspondingHost->getAddr() + ":" + m_endPoint + ".txt";
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

    m_startTime = other.m_startTime;

    m_isOpen = other.m_isOpen;

    m_inBuffer = std::move(other.m_inBuffer);
    m_currentMessageSize = other.m_currentMessageSize;
    m_bytesReceivedSoFar = other.m_bytesReceivedSoFar;
    m_numberOfTotalPackets = other.m_numberOfTotalPackets;
    m_receiverLastPacketAcked = other.m_receiverLastPacketAcked;

    m_logFilename = std::move(other.m_logFilename);
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
    log("Closing connection");
    std::scoped_lock<std::mutex> scopedLock(m_connectionLock);
    m_isOpen = false;
    stopTimeout();
    log("Finished closing connection");
}

void TCPConnection::resetConnection()
{
    log("Resetting connection");
    closeConnection();
    std::scoped_lock<std::mutex> scopedLock(m_connectionLock);
    auto msgBuffer = std::move(m_msgBuffer);
    *(this) = TCPConnection(m_correspondingHost, m_endPoint, m_estimatedRTT);
    m_msgBuffer = std::move(msgBuffer);
    m_isOpen = true;
    log("Finished resetting connection");
}


void TCPConnection::sendMessage(const std::vector<char>& msgBuffer, uint64_t repeatDelay)
{
    log("Beginning to send a message of size " + std::to_string(msgBuffer.size()));
    resetConnection();
    m_repeatDelay = repeatDelay;
    m_msgBuffer = msgBuffer;
    packetize(msgBuffer);
    m_startTime = std::chrono::high_resolution_clock::now();
    sendNextWindow();
    m_isTimeoutAllowed = true;
    startTimeout(getEstimatedTimeout());
}

void TCPConnection::sendMessage()
{
    auto repeateDelay = m_repeatDelay;
    m_connectionLock.unlock();
    resetConnection();
    m_connectionLock.lock();
    m_repeatDelay = repeateDelay;
    packetize(m_msgBuffer);
    sendNextWindow();

    m_isTimeoutAllowed = true;
    startTimeout(getEstimatedTimeout());
}

void TCPConnection::startSlowAgain()
{
    log("Slow starting again");
    m_ssthresh = std::max(m_cwnd/2, (uint64_t)1);
    m_cwnd = 1;
    m_lastPacketSent = m_senderLastPacketAcked;
}

void TCPConnection::timeoutBody(duration duration)
{
    bool dummy;
    if(!m_timeoutNotifier.wait_dequeue_timed(dummy, duration)) // timeout
    {
        log("Time out!");
        std::scoped_lock<std::mutex> scopedLock1(m_connectionLock);
        if(m_isTimeoutAllowed && m_isOpen)
        {
            log("Retransmitting...");
            startSlowAgain();
            sendNextWindow();
            startTimeout(getEstimatedTimeout());
            return;
        }
    }
}

void TCPConnection::startTimeout(duration duration)
{
    log("Starting timeout");
    m_timeoutThreads.push_back(std::thread(&TCPConnection::timeoutBody, this, std::move(duration)));
}

void TCPConnection::stopTimeout()
{
    log("Stopping timeout");
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
    log("New ack with packet id: " + std::to_string(packet->getPacketId()));
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
    if(!m_outBuffer[i+1].isAcked)
    {
        newRTT = std::chrono::high_resolution_clock::now() - m_outBuffer[i+1].timeSent;
        m_estimatedRTT = duration((uint64_t)(ALPHA*m_estimatedRTT.count() + BETA*newRTT.count()));
        log("New estimated RTT(nanoseconds): " + std::to_string(m_estimatedRTT.count()));
    }
}

void TCPConnection::packetize(const std::vector<char> &wholeMessage)
{
    log("Packetizing");
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
    log(std::to_string(m_outBuffer.size()-1) + " of total packets");
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
    m_lastPacketSent = m_senderLastPacketAcked + m_cwnd;
    m_lastPacketSent = std::min(m_lastPacketSent, m_outBuffer.size()-1);
    auto startingPoint = m_senderLastPacketAcked + 1;
    log("Transmitting with window size of: " + std::to_string(m_cwnd));
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
    log("File transmitted Completely!!!!\nRetransmitting in " + std::to_string(m_repeatDelay) + "ns ...");
    log("Time to send: " + std::to_string((std::chrono::high_resolution_clock::now()-m_startTime).count()) + "ns");
    stopTimeout();
    std::this_thread::sleep_for(std::chrono::nanoseconds(m_repeatDelay));
    sendMessage();
}

void TCPConnection::handleData(shared_ptr<Packet> packet)
{
    log("Handling data from " + packet->getSource() + " id: " + std::to_string(packet->getPacketId()));
    auto& body = packet->getBody();
    if(!m_inBuffer.size() && m_bytesReceivedSoFar == 0 && body.size() == 8) // empty buffer: first packet
    {
        m_startTime = std::chrono::high_resolution_clock::now();
        m_endPoint = packet->getSource();
        m_currentMessageSize = *((uint64_t*)body.data());
        m_numberOfTotalPackets = (m_currentMessageSize/Packet_Size) + (m_currentMessageSize%Packet_Size ? 1 : 0) + 1;
        m_inBuffer.push_back(packet);
    }
    else if(packet->getPacketId() == ((m_receiverLastPacketAcked+1)%maxPacketId)) //OK
    {
        m_inBuffer.push_back(packet);
        m_bytesReceivedSoFar += packet->getBody().size();
        if(m_currentMessageSize == m_bytesReceivedSoFar)
            onNewFileCompletelyReceived();
    }

    if(packet->getPacketId() == (m_receiverLastPacketAcked+1)%maxPacketId )
        m_receiverLastPacketAcked++;
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
    log("File received completely!!!");
    log("Time to receive: " + std::to_string((std::chrono::high_resolution_clock::now()-m_startTime).count()) + "ns");
    std::ofstream file("./log/" + m_correspondingHost->getAddr() + " from " + m_endPoint, std::ios_base::out | std::ios_base::binary);
    m_inBuffer.erase(m_inBuffer.begin()); // first packet is header
    for(auto& packet : m_inBuffer)
        file.write(packet->getBody().data(), packet->getBody().size());
    m_inBuffer.clear();
    m_bytesReceivedSoFar = 0;
    m_currentMessageSize = 0;
    m_numberOfTotalPackets = 0;
    m_receiverLastPacketAcked = 0;
}

