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

uint32_t ConfigServer::convertIpBinary(string ip)
{
    uint32_t result = 0;
	size_t bitwise = 24;
    for (size_t i = 0; i < 4; i++)
    {
		int      ipPart;
		size_t   index = 0;
        ipPart = stoi(ip, &index, 10);
        if (index > 3 || (i < 3 && ip[index] == '\0'))
			throw runtime_error(to_string(_lineNbr) + ": listen: invalid ipv4 address entered hostname");
        if (ipPart > 255)
			throw runtime_error(to_string(_lineNbr) + ": listen: too large ipv4 address entered");
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
            throw runtime_error(to_string(_lineNbr) + ": listen: port contains . character");
    }
    else if (line[index] == ':')
    {
        hostname = line.substr(0, skipHostname);
        line = line.substr(skipHostname + 1);
    }
	else
		throw runtime_error(to_string(_lineNbr) + ": listen: invalid character found after listen: " + line[index]);
    sockaddr_in ipv4;
    ipv4.sin_addr.s_addr =
        static_cast<in_addr_t>(htonl(convertIpBinary(hostname)));
		uint32_t port = stoi(line, &index);
    if (port == 0 || port > 65535)
        throw runtime_error(to_string(_lineNbr) + ": listen: invalid port entered for listen should be between 1 and 65535: " + to_string(port));
    ipv4.sin_port = htons(static_cast<uint16_t>(port));
    sockaddr in = *reinterpret_cast<sockaddr *>(&ipv4);
	in.sa_family = AF_INET;
    _hostAddress.insert({hostname, in});
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
