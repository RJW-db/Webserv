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

// void ConfigServer::addHostPort(string line, string hostname)
// {

// }

string ConfigServer::listenHostname(string line)
{ //to do should we store in addrinfo immediately? and how to handle fstream
    size_t   skipSpace = line.find_first_not_of(" \t\f\v\r");
    size_t   skipHostname = line.find_first_not_of("0123456789.", skipSpace);
    sockaddr in;
	string      hostname;
    if (line[skipHostname] == ';')
    {
        if (line.find('.', skipSpace) < skipHostname - skipSpace)
            throw runtime_error("listen port contains .");
		hostname = "0.0.0.0";
        // sockaddr_in ipv4;
        // ipv4.sin_addr.s_addr =
        //     static_cast<in_addr_t>(htonl(convertIpBinary("0.0.0.0")));
        // uint32_t port = stoi(line.substr(skipSpace));
        // if (port == 0 || port > 65535)
        //     throw runtime_error("invalid port entered for listen");
        // ipv4.sin_port = htons(static_cast<uint16_t>(port));
        // in = *reinterpret_cast<sockaddr *>(&ipv4);
        // _hostAddress.insert({"0.0.0.0", in});
        // return (line.substr(skipHostname + 1));
    }
    else if (line[skipHostname] == ':')
    {
        hostname = line.substr(skipSpace, skipHostname - skipSpace);
		// cout << hostname << endl;
        line = line.substr(skipHostname + 1);
		// cout << line << endl;

        // sockaddr_in ipv4;
        // ipv4.sin_addr.s_addr =
        //     static_cast<in_addr_t>(htonl(convertIpBinary(hostname)));
        // uint32_t port = stoi(line.substr(skipHostname + 1));
        // if (port == 0 || port > 65535)
        //     throw runtime_error("invalid port entered for listen");
        // ipv4.sin_port = htons(static_cast<uint16_t>(port));
        // in = *reinterpret_cast<sockaddr *>(&ipv4);
        // _hostAddress.insert({hostname, in});
        // size_t pos = line.find_first_not_of("0123456789", skipHostname + 1);
        // if (pos != string::npos && line[skipHostname + pos] == ';')
        //     throw runtime_error(
        //         "invalid character found after listen hostname and port");
        // return (line.substr(pos));
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
    in = *reinterpret_cast<sockaddr *>(&ipv4);
    _hostAddress.insert({hostname, in});
    size_t pos = line.find_first_not_of("0123456789 \t\f\v\r");
	if (line[pos] != ';' || pos == string::npos)
		throw runtime_error(
			"invalid character found after listen hostname and port");
    return (line.substr(pos));
}
