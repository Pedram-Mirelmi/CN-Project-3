#include <iostream>
#include <sstream>
#include <fstream>
#include "Network.h"
using namespace std;

std::vector<char> readFileContent(const std::string& filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if(!file)
        return std::vector<char>();
    std::vector<char> content(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(content.data(), content.size());
    file.close();
    return content;
}

void handleAdd(std::stringstream& commandStream, Network& network)
{
    string secondWord;
    commandStream >> secondWord;
    if(secondWord == "host")
    {
        string nodeAddr;
        while (true) {
            commandStream >> nodeAddr;
            network.addHost(nodeAddr);
            if(commandStream.tellg() == -1)
                break;
        }
    }
    else if(secondWord == "router")
    {
        string nodeAddr;
        while (true) {
            commandStream >> nodeAddr;
            network.addRouter(nodeAddr);
            if(commandStream.tellg() == -1)
                break;
        }
    }
    else if(secondWord == "link")
    {
        string addr1, addr2;
        uint64_t cost;
        commandStream >> addr1 >> addr2 >> cost;
        network.addLink(addr1, addr2, cost);
    }
}

void handleSend(std::stringstream& commandStream, Network& network)
{
    using namespace std;
    string sender, toWord, receiver, filename;
    size_t dummySize;
    commandStream >> sender >> toWord >> receiver >> filename;
    std::vector<char> dataBuffer;
    if(filename == "dummy")
    {
        commandStream >> dummySize;
        dataBuffer.reserve(dummySize);
        for(size_t i = 0; i < dummySize; i++)
            dataBuffer.push_back(i);
    }
    else
    {
        dataBuffer = readFileContent(filename);
        if(!dataBuffer.size())
        {
            std::cout << "file empty or not found" << std::endl;
            return;
        }
    }
    network.commandToSend(std::move(sender),
                          std::move(receiver),
                          std::move(dataBuffer),
                          std::move(filename));
}

void handleSet(std::stringstream& commandStream, Network& network)
{
    using namespace std;
    string nodeTypename, propery;
    uint64_t value;
    cin >> nodeTypename >> propery >> value;
    if(nodeTypename == "router")
    {
        if(propery == "delay")
            network.setRouterDelay(value);
        else if(propery == "FIFO")
            network.setRouterFifo(value);
    }

}

void runProgram()
{
    using namespace std;
    std::string command;
    Network network;
    while (true) {
        cout << ">> ";
        getline(std::cin, command);
        stringstream commandStream(command);

        string firstWord;
        commandStream >> firstWord;
        if(firstWord == "add") // add
        {
            handleAdd(commandStream, network);
        }
        else if(firstWord == "update") // update
        {
//            string secondWord;
//            commandStream >> secondWord; // link
//            if(secondWord != "link")
//            {
//                cout << "unknown command";
//                continue;
//            }
//            string addr1, addr2;
//            uint64_t costOrSd, td;
//            commandStream >> addr1 >> addr2 >> costOrSd;
//            if(commandStream.tellg() == -1)
//            {
//                network.updateLinkCost(addr1, addr2, costOrSd);
//            }
//            else
//            {
//                commandStream >> td;
//                network.temporarilyDownLink(addr1, addr2, costOrSd, td);
//            }

        }
        else if(firstWord == "remove") // remove
        {
//            string addr1, addr2;
//            commandStream >> addr1 >> addr2;
//            network.removeLink(addr1, addr2);
        }
        else if(firstWord == "log")
        {
            string secondWord, addr1, addr2;
            commandStream >> secondWord >> addr1 >> addr2;
            network.logLink(addr1, addr2);
        }
        else if(firstWord == "run")
        {
            string secondWord;
            network.run();
            network.waitForConverge();
        }
        else if(firstWord == "draw")
        {
            network.draw();
        }
        else if(firstWord == "show")
        {
            string secondWord, addr;
            commandStream >> secondWord >> addr;
            network.showTables(addr);
        }
        else if(firstWord == "shut")
        {
            string secondWord;
            commandStream >> secondWord;
            network.shutDown();
        }
        else if(firstWord == "send")
        {
            handleSend(commandStream, network);
        }
        else if(firstWord == "set")
        {
            handleSet(commandStream, network);
        }
        else
        {
            std::cout << "Unknown command. try again" << std::endl;
        }
    }
    cout << "editing runProgram function" << endl;
}

int main()
{
//    runProgram();
    std::cout << sizeof (void*) << std::endl;
    return 0;
}










