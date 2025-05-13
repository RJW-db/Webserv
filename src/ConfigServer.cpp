#include <ConfigServer.hpp>

ConfigServer::ConfigServer()
{
	
}

ConfigServer::ConfigServer(const ConfigServer &other)
{
	*this = other;
}


ConfigServer::~ConfigServer()
{
}

ConfigServer &ConfigServer::operator=(const ConfigServer &other)
{
	if (this != &other)
	{
		_hostAddress = other._hostAddress;
	}
	return (*this);
}

std::string ConfigServer::listenHostname(std::string line)
{
	std::size_t skipSpace = line.find_first_not_of(" \t\f\v\r");
	
}