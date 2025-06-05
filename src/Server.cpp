#include "Server.hpp"

Server::Server(ConfigServer &config) : _config(config) {}

void    Server::createListeners(vector<unique_ptr<Server>> &servers)
{
    for (auto &hostPort : _config.getPortHost())
    {
       ServerListenFD listenerFD(hostPort.first.c_str(), hostPort.second.c_str());
       _listeners.push_back(listenerFD.getFD());
    }
}