#pragma once
#include <string>
#include <mutex>
#include <list>
#include <thread>
#include "./Messages/Packet.h"
#include "./Nodes/Host.hpp"
class TCPConnection
{
    using string = std::string;
    string m_endPoint;
    uint64_t m_cwnd = 1;
    uint64_t m_ssthresh = -1; // max unsigned int

    std::vector<shared_ptr<Packet>> m_inBuffer, m_outBuffer;
    uint64_t m_lastPacketAcked, m_lastByteSent;
    std::vector<std::thread> m_timeoutThreads;
    moodycamel::BlockingConcurrentQueue<bool> m_timeoutNotifier;


    std::mutex m_connectionLock;
    shared_ptr<Host> m_correspondingHost;

    uint64_t m_currentMessageSize = 0, m_bytesReceivedSoFar = 0;
    bool m_isTimeoutActive = false;
public:
    static uint64_t Packet_Size;
public:
    TCPConnection(shared_ptr<Host>&& host, const string& endPoint);
    bool takePacket(shared_ptr<Packet> packet);

    // when it's sender:
    void sendMessage(const std::vector<char>& msgBuffer);
    void startSlowAgain();
    void timeoutBody(std::chrono::duration<long> duration);
    void startTimeout(std::chrono::duration<long> duration);
    void stopTimeout();
    void handleAck(shared_ptr<Packet> packet);
    void packetize(const std::vector<char>& wholeMessage);
    std::chrono::duration<long> getEstimatedTimeout();
    void sendNextWindow();

    // when it's receiver
    void handleData(shared_ptr<Packet> packet);
    void sendPacketAck(shared_ptr<Packet> packet);
    void onNewFileCompletelyReceived();
};



