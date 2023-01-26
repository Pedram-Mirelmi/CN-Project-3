#include "DVAlgController.h"

shared_ptr<DVAlgController> DVAlgController::instance;

DVAlgController::DVAlgController()
{

}

shared_ptr<DVAlgController> DVAlgController::getInstance()
{
    if(DVAlgController::instance)
        return DVAlgController::instance;
    else
        return DVAlgController::instance = shared_ptr<DVAlgController>(new DVAlgController());
}

DVAlgController::time_point DVAlgController::getStartTime()
{
    return m_startTime;
}

void DVAlgController::setStartTime(const time_point &time)
{
    m_startTime = time;
}

DVAlgController::time_point DVAlgController::getEndTime()
{
    return m_endTime;
}

void DVAlgController::inc()
{
    std::scoped_lock<std::mutex> scopedLock(m_counterLock);
    m_counter++;
}

void DVAlgController::dec()
{
    std::scoped_lock<std::mutex> scopedLock(m_counterLock);
    m_counter--;
    if(!m_counter)
        m_endTime = std::chrono::system_clock::now();
}

uint64_t DVAlgController::counter() const
{
    std::scoped_lock<std::mutex> scopedLock(m_counterLock);
    return m_counter;
}


