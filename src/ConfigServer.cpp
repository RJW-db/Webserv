#include <ConfigServer.hpp>

// Aconfig::~Aconfig()
// {}

ConfigServer::ConfigServer()
{
	
}

ConfigServer::ConfigServer(const ConfigServer &other) : Aconfig(other)
{
	*this = other;
}

ConfigServer::~ConfigServer(){ }

ConfigServer &ConfigServer::operator=(const ConfigServer &other)
{
	if (this != &other)
	{
		Aconfig::operator=(other);
		_portHost = other._portHost;
		_locations = other._locations;
	}
	return (*this);
}

bool ConfigServer::listenHostname(string &line)
{ 
	// to do should we store in addrinfo immediately? and how to handle fstream
	size_t skipHostname = line.find_first_not_of("0123456789.");
	size_t index =  line.find_first_not_of(" \t\f\v\r0123456789.");
    string hostname = "0.0.0.0";
    if (index == string::npos || line[index] == ';' )
    {
        if (line.find('.') < skipHostname)
            throw runtime_error(to_string(_lineNbr) + ": listen: port contains . character");
    }
    else if (line[index] == ':')
    {
        hostname = line.substr(0, skipHostname);
        line = line.substr(skipHostname + 1);
    }
	else
		throw runtime_error(to_string(_lineNbr) + ": listen: invalid character found after listen: " + line[index]);
	uint32_t port = stoi(line, &index);
    if (port == 0 || port > 65535)
        throw runtime_error(to_string(_lineNbr) + ": listen: invalid port entered for listen should be between 1 and 65535: " + to_string(port));
    _portHost.insert({line.substr(0, index), hostname});
	return (handleNearEndOfLine(line, index, "listen"));
}

// UTILITY FUNCTION

bool ConfigServer::serverName(string &line)
{
	if (!_serverName.empty())
		throw runtime_error(to_string(_lineNbr) + ": server_name: Parsing: tried setting server_name twice");
	size_t len = line.find_first_of(" \t\f\v\r;");
	if (len == string::npos)
	{
		_serverName = line;
		return (false);
	}
	_serverName = line.substr(0, len);
	return (handleNearEndOfLine(line, len, "server_name"));
}

void ConfigServer::setDefaultConf()
{
    //what to do with no error pages? do we create our own or just have one
    if (_root.empty())
        _root = "/var/www"; // what default root should we use?
    if (_clientBodySize == 0)
        _clientBodySize = 1024 * 1024;
    if (_autoIndex == autoIndexNotFound)
        _autoIndex = autoIndexFalse;
    if (_portHost.empty())
    {
        string tmp = "80;";
        listenHostname(tmp); //sets sockaddr ip to 0.0.0.0 and port to 80
    }
    for(auto &val : _locations)
	{
		Location &location = val.second;
        location.SetDefaultLocation(*this);
	}
    setDefaultErrorPages();
}

void ConfigServer::addLocation(const Location &location, string &path)
{
	if (!_locations.insert({path, location}).second)
	{
		throw runtime_error(to_string(_lineNbr) + ": addLocation: Parsing: tried adding location with same path twice: " + path);
	}
}

unordered_multimap<string, string> &ConfigServer::getPortHost(void)
{
	return _portHost;
}
unordered_map <string, Location> &ConfigServer::getLocations(void)
{
	return _locations;
}
string &ConfigServer::getServerName(void)
{
	return _serverName;
}

