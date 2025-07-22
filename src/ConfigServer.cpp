#include <ConfigServer.hpp>

ConfigServer::ConfigServer()
{
    
}

ConfigServer::ConfigServer(const ConfigServer &other) : AconfigServ(other)
{
    *this = other;
}

ConfigServer::~ConfigServer(){ }

ConfigServer &ConfigServer::operator=(const ConfigServer &other)
{
    if (this != &other)
    {
        AconfigServ::operator=(other);
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
    string strPort = line.substr(0, index);
    for (const auto& pair : _portHost)
    {
        if (pair.first == line.substr(0, index) && pair.second == hostname)
        {
            throw runtime_error(to_string(_lineNbr) + ": listen: Parsing: tried setting same port and hostname twice: " + strPort + " " + hostname);
        }
    }
    _portHost.insert({strPort, hostname});
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
    // what to do with no error pages? do we create our own or just have one
    if (_root.empty())
        _root = "/"; // what default root should we use?
    if (_root[_root.size() - 1] == '/')
        _root = _root.substr(0, _root.size() - 1);
    _root.insert(0, ".");
    if (_clientMaxBodySize == 0)
        _clientMaxBodySize = 1024 * 1024;
    if (_autoIndex == autoIndexNotFound)
        _autoIndex = autoIndexFalse;
    if (_portHost.empty())
    {
        string tmp = "80;";
        listenHostname(tmp); // sets sockaddr ip to 0.0.0.0 and port to 80
    }
    for (auto &val : _locations)
    {
        Location &location = val.second;
        location.SetDefaultLocation(*this);
	}
	//check if / path exists in locations if not create it
	bool found = false;
	for (auto it = _locations.begin(); it != _locations.end(); ++it)
	{
		if (it->first == "/")
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		Location location;
		location.SetDefaultLocation(*this);
		_locations.insert(_locations.begin(), {"/", location});
	}
    setDefaultErrorPages();
}

void ConfigServer::addLocation(const Location &location, string &path)
{
    for (auto it = _locations.begin(); it != _locations.end(); ++it)
    {
        pair<string, Location> &val = *it;
        if (val.first == path)
            throw runtime_error(to_string(_lineNbr) + ": addLocation: Parsing: tried adding location with same path twice: " + path);
        if (val.first.length() < path.length())
        {
            _locations.insert(it, {path, location});
            return;
        }
    }
    _locations.insert(_locations.end(), {path, location});
}

unordered_multimap<string, string> &AconfigServ::getPortHost(void)
{
    return _portHost;
}
vector<pair<string, Location>> &AconfigServ::getLocations(void)
{
    return _locations;
}
string &AconfigServ::getServerName(void)
{
    return _serverName;
}

AconfigServ::AconfigServ(const AconfigServ &other)
: Aconfig(other)
{
    *this = other;
}

AconfigServ &AconfigServ::operator=(const AconfigServ &other)
{
    if (this != &other)
    {
        Aconfig::operator=(other);
        _portHost = other._portHost;
        _locations = other._locations;
        _serverName = other._serverName;
    }
    return (*this);
}

int ConfigServer::getLineNbr(void) const
{
    return _lineNbr;
}