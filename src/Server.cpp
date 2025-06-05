#include "Server.hpp"

Server::Server(ConfigServer &config) : _config(config) {}


void    Server::createListeners(vector<unique_ptr<Server>> &servers)
{
	map<pair<const string, string>, int> listenersMade;
	int fd;
	for (auto &server : servers)
	{
		for (pair<const string, string> &hostPort : server->_config.getPortHost())
		{
			auto it = listenersMade.find(hostPort);
			if (it == listenersMade.end())
			{
				ServerListenFD listenerFD(hostPort.first.c_str(), hostPort.second.c_str());
				fd = listenerFD.getFD();
				listenersMade.insert({hostPort, fd});
			}
			else
			{
				fd = it->second;
			}
			server->addListener(fd);
		}
	}
}

void Server::addListener(int fd)
{
	_listeners.push_back(fd);
}
