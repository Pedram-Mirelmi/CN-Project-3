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
        std::getline(std::cin, command);
        std::stringstream commandStream(command);

        std::string firstWord;
        commandStream >> firstWord;
        if(firstWord == "add") // add
        {
            string secondWord;
            getline(commandStream, secondWord, ' ');
            if(secondWord == "host")
            {
                string nodeAddr;
                while (getline(commandStream, nodeAddr, ' ')) {
                    network.addHost(nodeAddr);
                }
            }
            else if(secondWord == "router")
            {
                string nodeAddr;
                while (getline(commandStream, nodeAddr, ' ')) {
                    network.addRouter(nodeAddr);
                }
            }
            else if(secondWord == "link")
            {
                string firstAddr, secondAddr;
                uint64_t cost;
                commandStream >> firstAddr >> secondAddr >> cost;

            }
        }
        else if(firstWord == "update") // update
        {
            string secondWord;
            commandStream >> secondWord; // link
            if(secondWord != "link")
            {
                cout << "unknown command";
                continue;
            }
            string addr1, addr2;
            uint64_t costOrSd, td;
            commandStream >> addr1 >> addr2 >> costOrSd;
            if(commandStream.tellg())
            {
                network.updateLinkCost(addr1, addr2, costOrSd);
            }
            else
            {
                commandStream >> td;
                network.temporarilyDownLink(addr1, addr2, costOrSd, td);
            }

        }
        else if(firstWord == "remove") // remove
        {
            string addr1, addr2;
            commandStream >> addr1 >> addr2;
            network.removeLink(addr1, addr2);
        }
        else if(firstWord == "log")
        {
            string secondWord, addr1, addr2;
            commandStream >> secondWord >> addr1 >> addr2;
            network.logLink(addr1, addr2);
        }
        else if(firstWord == "run")
        {
            network.run();
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
        else
        {
            std::cout << "Unknown command. try again" << std::endl;
        }
    }
}


int main()
{
    stringstream ss("test1 test2");
    string s;
    cout << ss.tellg() << endl;
    ss >> s;
    cout << s << endl;
    cout << ss.tellg() << endl;
    ss >> s;
    cout << s << endl;
    cout << ss.tellg() << endl;
    ss >> s;
    cout << s << endl;
    return 0;
}
