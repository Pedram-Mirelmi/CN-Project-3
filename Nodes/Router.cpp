#include <iostream>
#include <fstream>
#include "Router.hpp"
#include "Host.hpp"



Router::duration Router::defaultRouterDelay = Router::duration(0);
uint64_t Router::defaultBufferSize = 5;

Router::Router(const string &addr) :
    AbstractNode(addr), m_algController(NetworkController::getInstance())
{

}

void Router::log(const string &msg)
{
    std::scoped_lock<std::mutex> scopedLock(m_logLock);
    std::ofstream logFile(string("ROUTER: ") + m_addr, std::ios_base::app);
    logFile << msg << '\n';
    logFile.close();
}

void Router::startNode()
{
    log("Starting ...");
    m_thread = std::thread([this](){
        m_isRunning = true;
        m_algController->incRunningNodeCounter();
        for(auto& pair : m_routingTable)
        {
            broadCastNewLink(pair.first, pair.second.cost);
        }
        while (true)
        {
            shared_ptr<AbstractNetMessage> message;
            this->m_nodeQueue.wait_dequeue(message);
            if(m_mustStop)
                break;
            this->handleNewMessage(std::move(message));
        }
        m_mustStop = false;
        m_isRunning = false;
        m_algController->decRunningNodeCounter();
    });
}



void Router::handleNewMessage(shared_ptr<AbstractNetMessage> message)
{
    switch (message->getMessageType())
    {
    case AbstractNetMessage::ROUTING_MESSAGE:
    {
        auto routingMessage = std::dynamic_pointer_cast<RoutingMessage>(message);
        auto betweenNodeCost = m_links[routingMessage->getSender()->getAddr()].cost;
        updateRoutingTable(routingMessage->getDestination(), routingMessage->getCost()+betweenNodeCost, routingMessage->getSender());
        m_algController->decConvergeCounter();
        break;
    }
    case AbstractNetMessage::PACKET:
    {
        auto packet = std::dynamic_pointer_cast<Packet>(message);
        routeAndForwardPacket(packet);
        break;
    }
    default:
    {
        std::cout << "Unknown type of message received!" << std::endl;
    }
    }
}

bool Router::updateRoutingTable(const string& dest, uint64_t cost, shared_ptr<AbstractNode> nextHop)
{
    if(AbstractNode::updateRoutingTable(dest, cost, std::move(nextHop)))
    {
        broadCastNewLink(dest, cost);
        return true;
    }
    return false;
}

void Router::takeMessage(shared_ptr<AbstractNetMessage> message)
{
    log("Received a message");
    if(!m_mustStop)
    {
        if(m_fifoSize == (uint64_t)-1)
            m_nodeQueue.enqueue(std::move(message));
        else
            m_nodeQueue.try_enqueue(std::move(message));
    }
}

void Router::broadCastNewLink(string destination, uint64_t updatedCost)
{
    log("Broadcasting a link to " + destination + " with cost of " + std::to_string(updatedCost));
    if(true)
    {

        for(auto& pair : m_toHostsLinks)
        {
            pair.second.host->takeMessage(make_shared<RoutingMessage>(std::dynamic_pointer_cast<Router>(shared_from_this()), destination, updatedCost));
            m_algController->incConvergeCounter();
        }
        for(auto& pair : m_toRoutersLinks)
        {
            pair.second.router->takeMessage(make_shared<RoutingMessage>(std::dynamic_pointer_cast<Router>(shared_from_this()), destination, updatedCost));
            m_algController->incConvergeCounter();
        }
    }
}

void Router::routeAndForwardPacket(shared_ptr<Packet> packet)
{
    log("Routing a packet from " + packet->getSource() + " to " + packet->getDestination());
    auto destAddr = packet->getDestination();
    if(!m_routingTable.contains(destAddr))
    {
        std::cout << "Address not found" << std::endl;
        return;
    }

    std::chrono::high_resolution_clock::duration delay;
    {
        std::scoped_lock<std::mutex> scopedLock(m_routerLock);
        delay = m_nanosecDelay;
    }
    std::this_thread::sleep_for(m_nanosecDelay);
    m_routingTable[destAddr].nextHop->takeMessage(std::move(packet));
}

void Router::setDelay()
{
    setDelay(defaultRouterDelay.count());
}

void Router::setDelay(uint64_t nanosecends)
{
    log("Setting delay to " + std::to_string(nanosecends) + "ns");
    std::scoped_lock<std::mutex> scopedLock(m_routerLock);
    m_nanosecDelay = std::chrono::nanoseconds(nanosecends);
}

void Router::setBufferSize()
{
    setBufferSize(defaultBufferSize);
}

void Router::setBufferSize(u_int64_t size)
{
    log("Setting biffer size to " + std::to_string(size));
    if(size == (uint64_t)-1)
        m_fifoSize = size;
    else
        m_nodeQueue = moodycamel::BlockingConcurrentQueue<shared_ptr<AbstractNetMessage>>(size);
}


AbstractNode::NodeType Router::getType()
{
    return NodeType::ROUTER;
}


