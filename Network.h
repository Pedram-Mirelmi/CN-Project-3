#pragma once
#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>
#include "./Nodes/Host.hpp"
#include "./Nodes/Router.hpp"

#define NETWORK_DEBUGGING

using std::shared_ptr;
using std::make_shared;
class Network
{
    using string = std::string;
    struct Link
    {
        shared_ptr<AbstractNode> one, another;
        uint64_t cost;
    };

    typedef std::unordered_map<string, shared_ptr<Router>> RouterMap_t;
    typedef std::unordered_map<string, shared_ptr<Host>> HostMap_t;
private:
    struct DownLinkTask
    {
        std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
        std::thread sleepingThread;
    };
    shared_ptr<NetworkController> m_algController;
    std::vector<DownLinkTask> m_downLinkTasks;
    RouterMap_t m_routers;
    HostMap_t m_hosts;
public:
    Network();
    void addHost(const string& addr);

    void addRouter(const string& addr);

    void addLink(const string& addr1,
                 const string& addr2,
                 uint64_t cost);

    void updateLinkCost(const string& addr1,
                        const string& addr2,
                        uint64_t cost);

    void temporarilyDownLink(const string& addr1,
                             const string& addr2,
                             uint64_t downTimeFromNow,
                             uint64_t UpTimeFromDownTime);

    void removeLink(const string& addr1,
                    const string& addr2);

    void run();

    void shutDown();

    void waitForConverge();

    void draw();

    void logLink(const string& addr1,
                 const string& addr2);

    void showTables(const string& addr);

    void printTable(const AbstractNode::RoutingTable_t& table);

    void commandToSend(const string& sender,
                       const string& receiver,
                       const std::vector<char>& data,
                       uint64_t repeateDelay);

    uint64_t findLinkCost(const string& addr1, const string& addr2);

    void setRouterDelay(u_int64_t nanoseconds);

    void setRouterFifo(u_int16_t length);


    ~Network();
};
