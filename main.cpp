#include <iostream>
#include <sstream>
#include "Network.h"
using namespace std;

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
        else
        {
            std::cout << "Unknown command. try again" << std::endl;
        }
    }
    cout << "editing runProgram function" << endl;
}


int main()
{
    runProgram();
    return 0;
}
