#pragma once
#include <memory>
#include <unordered_map>
#include <thread>
#include "../concurrentqueue-master/blockingconcurrentqueue.h"
#include "../Messages/AbstractNetMessage.h"
using std::shared_ptr;

class Router;
class Host;



class AbstractNode : public std::enable_shared_from_this<AbstractNode>
{
public:
    using string = std::string;
    struct RoutingTableEntry
    {
        string destination;
        uint64_t cost;
        shared_ptr<AbstractNode> nextHop;
    };

    struct ToRouterLink
    {
        shared_ptr<Router> router;
        uint64_t cost;
    };
    struct ToHostLink
    {
        shared_ptr<Host> host;
        uint64_t cost;
    };
    typedef std::unordered_map<string, ToRouterLink> RouterLinkMap_t;
    typedef std::unordered_map<string, ToHostLink> HostLinkMap_t;
    typedef std::unordered_map<string, RoutingTableEntry>   RoutingTable_t ;

protected:
    RouterLinkMap_t m_toRoutersLinks;
    HostLinkMap_t m_toHostsLinks;

    RoutingTable_t m_routingTable;

    string m_addr;
    std::thread m_thread;
    moodycamel::BlockingConcurrentQueue<shared_ptr<AbstractNetMessage>> m_nodeQueue;
public:
    enum NodeType {ROUTER, HOST};
public:
    AbstractNode(const string& addr);
    virtual const string &getAddr() const;
    virtual void startNode() = 0;

    virtual void addToRouterLink(shared_ptr<Router> router, uint64_t cost) = 0;
    virtual void addToHostLink(shared_ptr<Host> host, uint64_t cost) = 0;

    virtual void updateToRouteLink(shared_ptr<Router> router, uint64_t cost) = 0;
    virtual void updateToHostLink(shared_ptr<Host> host, uint64_t cost) = 0;

    virtual void takeMessage(shared_ptr<AbstractNetMessage> message) = 0;

    virtual NodeType getType() = 0;
    const RoutingTable_t &getRoutingTable() const;
    const RouterLinkMap_t &getRoutersLinks() const;
    const HostLinkMap_t &getHostsLinks() const;
};

