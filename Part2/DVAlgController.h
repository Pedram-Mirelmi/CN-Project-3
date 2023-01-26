#pragma once
#include <memory>
#include <mutex>
#include <chrono>
using std::shared_ptr;
using std::make_shared;

class DVAlgController
{
    using time_point = std::chrono::system_clock::time_point;
    static shared_ptr<DVAlgController> instance;
    DVAlgController();
private:
    uint64_t m_counter = 0;
    mutable std::mutex m_counterLock;

    time_point m_startTime, m_endTime;

public:
    DVAlgController(const DVAlgController& other) = delete;
    DVAlgController& operator=(const DVAlgController& other) = delete;

    static shared_ptr<DVAlgController> getInstance();
    time_point getStartTime();
    void setStartTime(const time_point& time);
    time_point getEndTime();
    void inc();
    void dec();
    uint64_t counter() const;
};
