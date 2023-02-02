#include <iostream>
#include "AbstractNode.hpp"
#include "Router.hpp"
#include "Host.hpp"


AbstractNode::AbstractNode(const string& addr, bool limitedBuffer, uint64_t bufferLimit)
    : std::enable_shared_from_this<AbstractNode>(),
      m_addr(addr),
      m_isBufferLimited(limitedBuffer)
{
    if(m_isBufferLimited)
        m_nodeQueue = moodycamel::BlockingConcurrentQueue<shared_ptr<AbstractNetMessage>>(bufferLimit);
}

AbstractNode::~AbstractNode()
{
    std::cout << "destroying " << m_addr << std::endl;
}

const AbstractNode::string &AbstractNode::getAddr() const
{
    return m_addr;
}

void AbstractNode::stopNode()
{
    m_mustStop = true;
    m_nodeQueue.enqueue({});
}

void AbstractNode::clearQueue()
{
    m_nodeQueue = moodycamel::BlockingConcurrentQueue<shared_ptr<AbstractNetMessage>>(m_nodeQueue.size_approx());
}

void AbstractNode::addToRouterLink(shared_ptr<Router> router, uint64_t cost)
{
    m_toRoutersLinks[router->getAddr()] = {router, cost};
    m_links[router->getAddr()] = {router, cost};
    updateRoutingTable(router->getAddr(), cost, router);
}

void AbstractNode::addToHostLink(shared_ptr<Host> host, uint64_t cost)
{
    m_toHostsLinks[host->getAddr()] = {host, cost};
    m_links[host->getAddr()] = {host, cost};
    updateRoutingTable(host->getAddr(), cost, host);
}

bool AbstractNode::updateRoutingTable(const string &dest, uint64_t cost, shared_ptr<AbstractNode> nextHop)
{
    if(!m_routingTable.contains(dest))
    {
        m_routingTable[dest] = {dest, cost, nextHop};
        return true;
    }
    else if(m_routingTable.contains(dest) &&
        m_routingTable[dest].cost > cost)
    { // update should take place
        m_routingTable[dest] = {dest, cost, nextHop};
        return true;
    }
    return false;
}

void AbstractNode::takeMessage(shared_ptr<AbstractNetMessage> message)
{
    if(!m_mustStop)
    {
        m_nodeQueue.enqueue(message);
    }
}

void AbstractNode::sendPacket(shared_ptr<Packet> packet)
{
    auto& dest = packet->getDestination();
    if(m_routingTable.contains(dest))
    {
        m_routingTable[dest].nextHop->takeMessage(packet);
    }
    else
    {
        // TODO
    }
}

const AbstractNode::RoutingTable_t &AbstractNode::getRoutingTable() const
{
    return m_routingTable;
}

const AbstractNode::RouterLinkMap_t &AbstractNode::getRoutersLinks() const
{
    return m_toRoutersLinks;
}

const AbstractNode::HostLinkMap_t &AbstractNode::getHostsLinks() const
{
    return m_toHostsLinks;
}


const AbstractNode::LinkMap_t &AbstractNode::getLinks() const
{
    return m_links;
}
