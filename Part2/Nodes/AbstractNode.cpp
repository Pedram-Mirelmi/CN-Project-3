#include "AbstractNode.hpp"




AbstractNode::AbstractNode(const string& addr)
    : std::enable_shared_from_this<AbstractNode>(),
      m_addr(addr)
{

}

const AbstractNode::string &AbstractNode::getAddr() const
{
    return m_addr;
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
