#ifndef SERVER_HPP
#define SERVER_HPP
#include <ConfigServer.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <netinet/in.h>
#include <vector>
       #include <sys/types.h>
       #include <sys/socket.h>
       #include <netdb.h>

using namespace std;

class Server
{
public:
    Server(ConfigServer &config) : _config(config) {}
    ~Server() {}


private:
    ConfigServer &_config;
    
    vector<int> _listeners; // ip/port

    void    createListeners();
    // bool setupSocket();
    // void closeSocket();
};

void    Server::createListeners()
{
    for (auto &hostPort : _config._hostAddress)
    {
       ServerListenFD listenerFD(hostPort.second.c_str(), hostPort.first.c_str());
       _listeners.push_back(listenerFD.getFD());
    }
}


#endif