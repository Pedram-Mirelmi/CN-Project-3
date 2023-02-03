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
    constexpr const static float ALPHA = 0.8;
    constexpr const static float BETA = 0.2;
    string m_endPoint;
    shared_ptr<Host> m_correspondingHost;

    static uint16_t maxPacketId;

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
    std::vector<char> m_msgBuffer;
    std::vector<OutBufferElement> m_outBuffer;
    uint64_t m_senderLastPacketAcked = 0, m_lastPacketSent = 0;
    std::vector<std::thread> m_timeoutThreads;
    moodycamel::BlockingConcurrentQueue<bool> m_timeoutNotifier;
    std::mutex m_connectionLock;
    bool m_isTimeoutAllowed = false;
    uint64_t m_repeatDelay;

    time_point m_startTime;


    bool m_isOpen = true;


    // when it's receiver
    std::vector<shared_ptr<Packet>> m_inBuffer;
    uint64_t m_currentMessageSize = 0;
    uint64_t m_bytesReceivedSoFar = 0;
    uint64_t m_numberOfTotalPackets = 0;
    uint64_t m_receiverLastPacketAcked = 0;


    string m_logFilename;
    std::mutex m_logLock;
    void log(const string& msg);
public:
    static uint64_t Packet_Size;

public:
    TCPConnection() = default;
    TCPConnection(shared_ptr<Host> host, const string& endPoint, duration initRTT);
    TCPConnection(const TCPConnection& other) = delete;
    TCPConnection& operator=(const TCPConnection& other) = delete;
    TCPConnection(TCPConnection&& other);
    TCPConnection& operator=(TCPConnection&& other);

    void takePacket(shared_ptr<Packet> packet);
    void closeConnection();
    void resetConnection();
    TCPConnection::duration getEstimatedTimeout();


    // when it's sender:
    void sendMessage(const std::vector<char>& msgBuffer, uint64_t repeatDelay);
    const string &getEndPoint() const;

private:
    void sendMessage();
    void startSlowAgain();
    void timeoutBody(duration duration);
    void startTimeout(duration duration);
    void stopTimeout();
    void handleAck(shared_ptr<Packet> packet);
    void measureRTT(shared_ptr<Packet> packet);
    void packetize(const std::vector<char>& wholeMessage);
    void sendNextWindow();
    void onDataSentCompletely();

    // when it's receiver
    void handleData(shared_ptr<Packet> packet);
    void sendPacketAck(shared_ptr<Packet> packet);
    void onNewFileCompletelyReceived();
};



