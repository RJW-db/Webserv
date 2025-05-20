#include <ConfigServer.hpp>

ConfigServer::ConfigServer()
{
	
}

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

string ConfigServer::error_page(string line)
{
	if (line[0] == '/')
	{
		// check vector list and add all to map with page
	}
	uint32_t error_page = stoi(line); // check for too high and if invalid character found
	(void)error_page;
	return "nothing";
}

string ConfigServer::root(string line, bool &findColon)
{
	size_t lenRoot = line.find_first_of(" \t\f\v\r;");
	if (lenRoot == string::npos)
	{
		findColon = false;
		_root = line;
		return line;
	}
	_root = line.substr(0, lenRoot);
	size_t indexColon = line.find(";", 0);
	if (indexColon != string::npos)
	{
		findColon = true;
		return line.substr(indexColon + 1);
	}
	findColon = false;
	return line;
}

string ConfigServer::listenHostname(string line, bool &findColon)
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
    uint32_t port = stoi(line);
    if (port == 0 || port > 65535)
        throw runtime_error("invalid port entered for listen");
    ipv4.sin_port = htons(static_cast<uint16_t>(port));
    sockaddr in = *reinterpret_cast<sockaddr *>(&ipv4);
    _hostAddress.insert({hostname, in});
    size_t pos = line.find_first_not_of("0123456789 \t\f\v\r");
    if (pos == string::npos) // didn't find any invalid character after port
	{
		findColon = false;
		return (line);
	}
    else if (line[pos] != ';')
        throw runtime_error(
            "invalid character found after listen hostname and port");
    else
        findColon = true;
    return (line.substr(pos));
}
