#pragma once
#include <string>
#include <mutex>
#include <list>
#include <thread>
#include "./Messages/Packet.h"
#include "./concurrentqueue-master/blockingconcurrentqueue.h"
class Host;

using std::shared_ptr;
using std::make_shared;


class TCPConnection
{
    using string = std::string;
    using duration = std::chrono::high_resolution_clock::duration;
    using time_point = std::chrono::high_resolution_clock::time_point;
    string m_endPoint;
    shared_ptr<Host> m_correspondingHost;

    // when it's a sender:
    struct OutBufferElement
    {
        shared_ptr<Packet> packet;
        bool isAcked = false;
        bool isSent = false;
        time_point timeSent;
    };

    uint64_t m_cwnd = 1;
    uint64_t m_ssthresh = -1; // max unsigned int
    duration m_estimatedRTT;
    std::vector<OutBufferElement> m_outBuffer;
    uint64_t m_lastPacketAcked, m_lastByteSent;
    std::vector<std::thread> m_timeoutThreads;
    moodycamel::BlockingConcurrentQueue<bool> m_timeoutNotifier;
    std::mutex m_timeoutLock;
    bool m_isTimeoutActive = false;




    // when it's receiver
    std::vector<shared_ptr<Packet>> m_inBuffer;
    uint64_t m_currentMessageSize = 0, m_bytesReceivedSoFar = 0;
public:
    static uint64_t Packet_Size;

public:
    TCPConnection() = default;
    TCPConnection(shared_ptr<Host>&& host, const string& endPoint, duration initRTT);
    TCPConnection(TCPConnection&& other);
    TCPConnection& operator=(TCPConnection&& other);

    bool takePacket(shared_ptr<Packet> packet);

    // when it's sender:
    void sendMessage(const std::vector<char>& msgBuffer);
    void startSlowAgain();
    void timeoutBody(std::chrono::duration<long> duration);
    void startTimeout(std::chrono::duration<long> duration);
    void stopTimeout();
    void handleAck(shared_ptr<Packet> packet);
    void measureRTT(shared_ptr<Packet> packet);
    void packetize(const std::vector<char>& wholeMessage);
    std::chrono::duration<long> getEstimatedTimeout();
    void sendNextWindow();

    // when it's receiver
    void handleData(shared_ptr<Packet> packet);
    void sendPacketAck(shared_ptr<Packet> packet);
    void onNewFileCompletelyReceived();
};



