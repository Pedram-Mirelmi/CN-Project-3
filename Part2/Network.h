#pragma once
#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>
#include "./Nodes/Host.hpp"
#include "./Nodes/Router.hpp"

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
    std::vector<DownLinkTask> m_downLinkTasks;
    RouterMap_t m_routers;
    HostMap_t m_hosts;
public:
    void addHost(const string& addr)
    {
        if(m_routers.contains(addr) || m_hosts.contains(addr))
        {
            std::cout << "Address " << addr << " is already in use" << std::endl;
            return;
        }
        m_hosts[addr] = make_shared<Host>(addr); // the only place where we construct a host
    }

    void addRouter(const string& addr)
    {
        if(m_routers.contains(addr) || m_hosts.contains(addr))
        {
            std::cout << "Address " << addr << " is already in use" << std::endl;
            return;
        }
        m_routers[addr] = make_shared<Router>(addr); // the only place where we construct a router
    }

    void addLink(const string& addr1,
                 const string addr2,
                 uint64_t cost)
    {
        if(!m_routers.contains(addr1) && !m_hosts.contains(addr1))
        {
            std::cout << "address " << addr1 << " is unknown" << std::endl;
            return;
        }
        if(!m_routers.contains(addr2) && !m_hosts.contains(addr2))
        {
            std::cout << "address " << addr2 << " is unknown" << std::endl;
            return;
        }
        if(m_hosts.contains(addr1)) // first is host
        {
            if(!m_routers.contains(addr2))
            {
                std::cout << "Cannot connect a Host to Host" << std::endl;
            }
            m_hosts[addr1]->addToRouterLink(m_routers[addr2], cost);
            m_routers[addr2]->addToHostLink(m_hosts[addr1], cost);
        }
        if(m_hosts.contains(addr2))
        {
            m_hosts[addr2]->addToRouterLink(m_routers[addr1], cost);
            m_routers[addr1]->addToHostLink(m_hosts[addr2], cost);
        }
        m_routers[addr1]->addToRouterLink(m_routers[addr2], cost);
        m_routers[addr2]->addToRouterLink(m_routers[addr1], cost);
    }

    void updateLinkCost(const string& addr1,
                        const string& addr2,
                        uint64_t cost)
    {
        addLink(addr1, addr2, cost);
    }

    void temporarilyDownLink(const string& addr1,
                             const string& addr2,
                             uint64_t downTimeFromNow,
                             uint64_t UpTimeFromDownTime)
    {
        if((!m_hosts.contains(addr1) && !m_routers.contains(addr1))  ||
           (!m_hosts.contains(addr2) && !m_routers.contains(addr2)))
        {
            std::cout << "Unknown address " << addr1 << std::endl;
            return;
        }
        auto cost = findLinkCost(addr1, addr2);
        if(cost)
        {
            DownLinkTask task;
            task.startTime = std::chrono::system_clock::now();
            task.sleepingThread = std::thread([=]
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(downTimeFromNow));
                    removeLink(addr1, addr2);
                    std::this_thread::sleep_for(std::chrono::milliseconds(UpTimeFromDownTime));
                    addLink(addr1, addr2, cost);
                }
            );
        }
        else
        {
            std::cout << "There is no link between these two" << std::endl;
        }

    }

    void removeLink(const string& addr1,
                    const string& addr2)
    {

    }

    void run()
    {

    }

    void draw()
    {

    }

    void logLink(const string& addr1,
                 const string& addr2)
    {

    }

    void showTables(const string& addr)
    {
        if(m_hosts.contains(addr))
            printTable(m_hosts[addr]->getRoutingTable());
        else if(m_routers.contains(addr))
            printTable(m_routers[addr]->getRoutingTable());
        else
            std::cout << "Unknown addr" << std::endl;

    }

    void printTable(const AbstractNode::RoutingTable_t& table)
    {
        std::cout << "=======================================================" << std::endl;
        for(auto& pair: table)
        {
            std::cout << std::setw(20) << pair.first
                      << std::setw(10) << pair.second.cost
                      << std::setw(20) << pair.second.nextHop->getAddr()
                      << std::endl;
        }
        std::cout << "=======================================================" << std::endl;
    }

    uint64_t findLinkCost(const string& addr1, const string& addr2)
    {
        if(m_hosts.contains(addr1))
        {
            if(m_hosts[addr1]->getRoutersLinks().contains(addr2))
                return m_hosts[addr1]->getRoutersLinks().at(addr2).cost;
            if(m_hosts[addr1]->getHostsLinks().contains(addr2))
                return m_hosts[addr1]->getHostsLinks().at(addr2).cost;
            else
                return 0;
        }
        else // addr1 is router
        {
            if(m_routers[addr1]->getRoutersLinks().contains(addr2))
                return m_routers[addr1]->getRoutersLinks().at(addr2).cost;
            if(m_routers[addr1]->getHostsLinks().contains(addr2))
                return m_routers[addr1]->getHostsLinks().at(addr2).cost;
            else
                return 0;
        }
    }
};
