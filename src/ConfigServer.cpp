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
		_hostAddress = other._hostAddress;
		_locations = other._locations;
	}
	return (*this);
}

static uint32_t convertIpBinary(string ip)
{
    uint32_t result = 0;
	size_t bitwise = 24;
    for (size_t i = 0; i < 4; i++)
    {
		int      ipPart;
		size_t   index = 0;
        ipPart = stoi(ip, &index, 10);
        if (index > 3 || (i < 3 && ip[index] == '\0'))
			throw runtime_error("invalid ipv4 address entered hostname");
        if (ipPart > 255)
			throw runtime_error("too large ipv4 address entered");
        if (i < 3)
			ip = ip.substr(index + 1);
        result += ipPart << bitwise;
        bitwise -= 8;
    }
    return (result);
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
            throw runtime_error("listen port contains .");
    }
    else if (line[index] == ':')
    {
        hostname = line.substr(0, skipHostname);
        line = line.substr(skipHostname + 1);
    }
	else
		throw runtime_error("invalid character found after listen");
    sockaddr_in ipv4;
    ipv4.sin_addr.s_addr =
        static_cast<in_addr_t>(htonl(convertIpBinary(hostname)));
		uint32_t port = stoi(line, &index);
    if (port == 0 || port > 65535)
        throw runtime_error("invalid port entered for listen");
    ipv4.sin_port = htons(static_cast<uint16_t>(port));
    sockaddr in = *reinterpret_cast<sockaddr *>(&ipv4);
	in.sa_family = AF_INET;
    _hostAddress.insert({hostname, in});
	return (handleNearEndOfLine(line, index, "listenHostname"));
}

// UTILITY FUNCTION
bool handleNearEndOfLine(string &line, size_t pos, string err)
{
	size_t k = line.find_first_not_of(" \t\f\v\r", pos);
	if (k == string::npos)
	{
		return false;
	}
	if (line[k] != ';')
	{
		throw runtime_error(err + ": invalid character before semi colon");
	}
	line = line.substr(k + 1);
	return true;
}

bool ConfigServer::serverName(string &line)
{
	if (!_serverName.empty())
		throw runtime_error("Parsing: tried creating second upload_store");
	size_t len = line.find_first_of(" \t\f\v\r;");
	if (len == string::npos)
	{
		_serverName = line;
		return (false);
	}
	_serverName = line.substr(0, len);
	return (handleNearEndOfLine(line, len, "server_name"));
}