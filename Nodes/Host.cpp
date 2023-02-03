#include <iostream>
#include <fstream>
#include "Host.hpp"
#include "Router.hpp"

Host::duration Host::initRTT;

Host::Host(const string &addr)
    : AbstractNode(addr), m_algController(NetworkController::getInstance())
{
    m_logFilename = "./log/Host:" + m_addr + ".txt";
}

void Host::addTcpConnection(const string &endPoint, duration initRTT)
{
    log("Adding a connection to " + endPoint);
    if(!m_connections.contains(endPoint))
    {
        m_connections[endPoint] = TCPConnection(std::dynamic_pointer_cast<Host>(shared_from_this()), endPoint, initRTT);
    }
}

void Host::log(const string &msg)
{
    std::scoped_lock<std::mutex> scopedLock(m_logLock);
    std::ofstream logFile(m_logFilename, std::ios_base::app);
    logFile << msg << '\n';
    logFile.close();
}

void Host::startNode()
{
    log("Starting ...");
    for(auto& connection : m_connections)
        connection.second = TCPConnection(std::dynamic_pointer_cast<Host>(shared_from_this()),
                                          connection.second.getEndPoint(),
                                          connection.second.getEstimatedTimeout());
    m_thread = std::thread([this](){
        this->m_isRunning = true;
        m_algController->incRunningNodeCounter();
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

void Host::stopNode()
{
    log("Stopping ...");
    AbstractNode::stopNode();
    for(auto& connection : m_connections)
        connection.second.closeConnection();
}

void Host::handleNewMessage(shared_ptr<AbstractNetMessage> message)
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
            auto packet = std::dynamic_pointer_cast<Packet>(std::move(message));
            handlePacket(std::move(packet));
            break;
        }
        default:
        {
            std::cout << "unknown type of message received!" << std::endl;
        }

    }
}

void Host::sendPacket(shared_ptr<Packet> packet)
{
    log("Sending a packet to " + packet->getDestination());
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


void Host::handlePacket(shared_ptr<Packet> packet)
{
//    log("Handling a packet from " + packet->getSource() +);
    if(m_connections.contains(packet->getSource()))
        m_connections[packet->getSource()].takePacket(std::move(packet));
    else
    {
        std::cout << "contains: " << m_connections.contains(packet->getSource()) << std::endl;
        std::cout << "Unrelated Packet from " << packet->getSource() << std::endl;
    }
}

AbstractNode::NodeType Host::getType()
{
    return NodeType::HOST;
}

void Host::sendMessageTo(const string& receiver, const std::vector<char>& data, uint64_t repeateDelay)
{
    log("Sending data of size " + std::to_string(data.size()) + " to " + receiver);
    if(m_connections.contains(receiver))
    {
        m_connections[receiver].sendMessage(data, repeateDelay);
    }
    else
    {
        std::cout << "No Connection with " << receiver << std::endl;
    }
}

