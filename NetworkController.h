#pragma once
#include <memory>
#include <mutex>
#include <chrono>
using std::shared_ptr;
using std::make_shared;

class NetworkController
{
    using time_point = std::chrono::high_resolution_clock::time_point;
    static shared_ptr<NetworkController> instance;
    NetworkController();
private:
    uint64_t m_algConvergecounter = 0, m_runningNodeCounter = 0;
    mutable std::mutex m_algConvergerCounterLock, m_runningNodeCounterLock;

    time_point m_startTime, m_endTime;

public:
    NetworkController(const NetworkController& other) = delete;
    NetworkController& operator=(const NetworkController& other) = delete;

    static shared_ptr<NetworkController> getInstance();
    time_point getAlgStartTime();
    void setStartAlgTime(const time_point& time);
    time_point getAlgEndTime();
    void incConvergeCounter();
    void decConvergeCounter();
    uint64_t getAlgConvergeCounter() const;

    void incRunningNodeCounter();
    void decRunningNodeCounter();
};
