# The Code
For some reseon we start with the code its self. 

### To maintain the whole of the network we defined a class names `Network` that holds all the nodes including routers and hosts and links between them. 
The `Network` class has individual methods for each command:

- `$add host <host names> ... ` ==> `Network::addHost(addr)`
- `$add router <router names> ...` ==> `Network::addRouter(addr)`
- `$add link node1 node2 cost` ==> `Network::addlink(addr1, addr1, cost)`
- `set router [delay|buffer|drop-rate] <value>` ==> `Network::setRouter[Delay|Buffer|DropRate](value)`
- `set packet size` ==> `Network::setPacketSize(value)`
- `run` ==> `Network::run()`
- `show tables <ip>` ==> `Network::showTables(ip)`
- `shut down` ==> `Network::shutDown()`
- `send <sender> to <receiver> [dummy|<filename>] [dummy size] <repeate delay>` ==> `Network::commandToSend(sender, receiver, dataBuffer, repeateDelay)`

## Messages

Since we have **Distance Vector** algorithm(1) and TCP(2) simulations we need 2 types of messages which will inherit from a `AbstractNetMessage` class:
### `AbstractNetMessage`

This is actually a simple class that has a `MessageType`
and a pure virtual method `getMessageType()` to access that type

### `RoutingMessage`

The routing message is actually a data class containing needed information for **Distance Vector** algorithm like:

- `m_sender`
- `m_destination`
- `m_cost`
- and some setters and getters

## Nodes
### `AbstractNode`
Now for managing nodes we have a basic `AbstractNode` with all the basic functionalities for a node in this project like:

- `getAddr()`: getting the ip address
- `startNode()`
- `stopNode()`
- `clearQueue()`: clear the buffer
- `joinThread()`: join the thread of the node
- `addToRouterLink(router, cost)`: add a link to a router
- `addToHostLink(Host, cost)`: add a link to a Host
- `updateRoutingTable(dest, cost, nextHop)`: update routing table
- `takeMessage(msg)`: take a message and place in buffer
- and some other getter methods
- `handleNewMessage(msg)`: to handle a new arrived message
- `log(msg)`: the log function

Now from that we have main nodes(`Router` and `Host`)
### `Host`
For the host we have following methods
- `addTCPConnection(endPoint, initRTT)`: to establish a TCP connection with the endpoint
- `sendPacket(packet)`: used by host's TCP connections to send packets to each other
- `sendMessageTo(receiver, msgBuffer)`: used by `Network` class to start a transmitting process
- `handlePacket(packet)`: handles a new arrived packet
- and some other overridden methods

### `Router`

For the `Router` class we have:
- `broudCastNewLink(dest, cost)`: to broadcast a new link used in **Distance Vector** algorithm
- `routeAndForwardPacket(packet)`: the main function of the router
- And some other setter functions

## `NetworkController` a helper class

The `NetworkController` is a helper singleton class that all nodes have access to its instance.
The class provides a counter of the `RoutingMessages` in going in the network to keep track of the **Distance Vector** algorithm and figure out when it stops. How? just by keeping track of a simple counter `m_algConvergeCounter` that every node in the network can increment or decrement it in a **thread-safe** manner. Each router simply increments `n` time it whenever it broadcasts a link to `n` other nodes. And any other node decrement it at the **end** of processing a `RoutingMessage`

Also when it comes to shutting down the network and nodes i similarily keep another counter `m_runningNodeCounter` to figure out when all nodes are off

It also sets the timer when the algorithm is finishied by setting a timer whenever the counter reaches to zero.

# Part 1: The TCP and Slow Start

We gathered all the code needed for the TCP simulation and Slow Start algorithm in one class (`TCPConnection`)

Each Host keeps a `std::unordered_map` to keep its connection with other hosts. we identify each connection with its other **end point**.

Let's first take a look at the class

## `TCPConnection` class

From a threading point of view, at each time only two threads are running methods of this class.

One is comming from the corresponding host that owns this connection and is dequeuing from its buffer constantly and hand it over to the connection object that will mainly call `handleData(packet)` method(if it's a receiver and the packet is not **Ack**) or `handleAck(packet)` method in case the connection is sender and the packet is an **Ack**.

The other thread is the timeout thread that is on and running(`timeoutBody` method) most of the time.

The complexity is when the connection is a sender. To avoid crashes due to race conditions and other problems due to multithreading we took a lock `m_connectionLock` and a flag `m_isTimeoutAllowed` 

The challenge is where whenever we send a window size of packets and start a timeout and whenever we receiver every Ack of those packets we cancell the timeout and send the next window and start another timeout. The problem is where these two happens at the same time. To avoid this problem we lock the `m_connectionLock` in each 2 threads. 
- In the **handleAck** case we check to see if the ack is the next "**not acked packet**". If so, we increment the `m_lastPacketAcked`, set the corresponding packet in buffer to `isAcked=true`, measure and estimate RTT, unlock the lock and exit. If not we do nothing. Also if the ack is for the last packet sent we cancell the timeout and send the next window size of packets and start another timeout. **Note that whenever we start a timeout we set `m_isTimeoutAllowed=true`**

- For the **timeout** case, we actually have two things keeping timeout from retransmitting. 
    - First the notifier that is just a blocking queue and we wait for some amount of time to dequeue and in case of failiar we pass. 
    - Second we try to lock the `m_connectionLock` and then check the `m_isTimeoutAllowed` flag. Usually another threads holding the lock, sets this flag to `false` and releases the lock for a moment so the timeout doesn't retransmit and return from the function 
    - Third if the flag is set to `true` we do the retransmittion and start another timeout and return.

**Pay attention that anytime that an event means a congestion we call the `slowStartAgain()` and reset the `m_cwnd`, `m_ssthreash` and `m_lastPacketSent=m_lastPacketAcked` (the last one is for the GO-BACK-N algorithm)**



# Simulations

## Simulation #1 
#### Parameters:
- router FIFO buffer size = inf
- packetsize = 10
- router service delay per packet = 0
- router drop rate = 0.0 (independent of full buffer)

### Terminal:
![](./Doc%20files/Simulation1-CMD-fifo%3Dinf%2Cpacketsize%3D10%2Cdelay%3D0.png)

### Receiver log file: 

**Time spent for receiver: 7,163,332,533 ns**


![](./Doc%20files/Simulation1-RecvLog-fifo%3Dinf%2Cpacketsize%3D10%2Cdelay%3D0ms.png) 

### Sender log file:

**Time spend for sender: 7,178,081,435 ns**

![](./Doc%20files/Simulation1-SendLog-fifo%3Dinf%2Cpacketsize%3D10%2Cdelay%3D0.png)

### Diff:

**All Ok!**

![](./Doc%20files/Simulation1-Diff-fifo%3Dinf%2Cpacketsize%3D10%2Cdelay%3D0.png)


## Simulation #2
#### Parameters:
- router FIFO buffer size = 5
- packet size = 10
- delay = 1ms
- router drop rate = 0.1 (independent of full buffer)

### Terminal
![](./Doc%20files/Simulation2-CMD-fifo%3D5%2Cpacketsize%3D10%2Cdelay%3D1ms%2Cdrop-rate%3D0.1.png)

### Receiver log file: 


**Time spent for receiver: 60,655,557,265 ns**


![](./Doc%20files/Simulation2-RecvLog-fifo%3D5%2Cpacketsize%3D10%2Cdelay%3D1ms%2Cdrop-rate%3D0.1.png)

### Sender log file:

**Time spent for sender: 60,679,495,256 ns**

![](./Doc%20files/Simulation2-SendLog-fifo%3D5%2Cpacketsize%3D10%2Cdelay%3D1ms%2Cdrop-rate%3D0.1.png)

### Diff

**All Ok**

![](./Doc%20files/Simulation2-Diff-fifo%3D5%2Cpacketsize%3D10%2Cdelay%3D1ms%2Cdrop-rate%3D0.1.png)

