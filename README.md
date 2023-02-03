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
The class provides a counter of the `RoutingMessages` in going in the network to keep track of the **Distance Vector** algorithm and figure out when it stops. How? just by keeping track of a simple counter `m_algConvergeCounter` that every node in the network can increment or decrement it in a **thread-safe** manner 

Also when it comes to shutting down the network and nodes i similarily keep another counter `m_runningNodeCounter` to figure out when all nodes are off

It also sets the timer when the algorithm is finishied by setting a timer whenever the counter reaches to zero.

