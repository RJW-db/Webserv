#include <ConfigServer.hpp>

// ConfigServer::ConfigServer(std::fstream &fs) : _fs(fs)
// {
	
// }

ConfigServer::ConfigServer(const ConfigServer &other)
{
	*this = other;
}


ConfigServer::~ConfigServer(){ }

ConfigServer &ConfigServer::operator=(const ConfigServer &other)
{
	if (this != &other)
	{
		_hostAddress = other._hostAddress;
	}
	return (*this);
}

std::string ConfigServer::listenHostname(std::string line)
{ //to do should we store in addrinfo immediately? and how to handle fstream
	std::size_t skipSpace = line.find_first_not_of(" \t\f\v\r");
	std::size_t skipHostname = line.find_first_not_of("0123456789.", skipSpace);
	if (line[skipHostname + skipSpace] == ';')
	{
		if (line.find('.', skipSpace) < skipHostname)
			throw std::runtime_error("listen port contains .");
		_hostAddress.insert({"0.0.0.0", static_cast<uint16_t>(std::stoi(line.substr(skipSpace)))});
		return (line.substr(skipSpace + skipHostname + 1));
	}
	else if (line[skipHostname + skipSpace] == ':')
	{
		std::string hostname = line.substr(skipSpace, skipHostname - skipSpace);
		_hostAddress.insert({hostname, std::stoi(line.substr(skipSpace + skipHostname + 1))}); //to do check for missing port
		std::size_t pos = line.find_first_not_of("0123456789", skipSpace + skipHostname + 1);
		if (pos != std::string::npos && line[skipSpace + skipHostname + pos] == ';')
			throw std::runtime_error("invalid character found after listen hostname and port");
		return (line.substr(skipSpace + skipHostname + pos));
	}
	else
		throw std::runtime_error("invalid character found after listen");
}