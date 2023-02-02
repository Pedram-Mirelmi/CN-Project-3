#include "NetworkController.h"

shared_ptr<NetworkController> NetworkController::instance;

NetworkController::NetworkController()
{

}

shared_ptr<NetworkController> NetworkController::getInstance()
{
    if(NetworkController::instance)
        return NetworkController::instance;
    else
        return NetworkController::instance = shared_ptr<NetworkController>(new NetworkController());
}

NetworkController::time_point NetworkController::getAlgStartTime()
{
    return m_startTime;
}

void NetworkController::setStartAlgTime(const time_point &time)
{
    m_startTime = time;
}

NetworkController::time_point NetworkController::getAlgEndTime()
{
    return m_endTime;
}

void NetworkController::incConvergeCounter()
{
    std::scoped_lock<std::mutex> scopedLock(m_algConvergerCounterLock);
    m_algConvergecounter++;
}

void NetworkController::decConvergeCounter()
{
    std::scoped_lock<std::mutex> scopedLock(m_algConvergerCounterLock);
    m_algConvergecounter--;
    if(!m_algConvergecounter)
        m_endTime = std::chrono::system_clock::now();
}

uint64_t NetworkController::getAlgConvergeCounter() const
{
    std::scoped_lock<std::mutex> scopedLock(m_algConvergerCounterLock);
    return m_algConvergecounter;
}

void NetworkController::incRunningNodeCounter()
{
    std::scoped_lock<std::mutex> scopedLock(m_runningNodeCounterLock);
    m_runningNodeCounter++;
}

void NetworkController::decRunningNodeCounter()
{
    std::scoped_lock<std::mutex> scopedLock(m_runningNodeCounterLock);
    m_runningNodeCounter--;
}


