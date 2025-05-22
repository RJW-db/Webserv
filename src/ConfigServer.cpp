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
		ErrorCodesWithPage = other.ErrorCodesWithPage;
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

string ConfigServer::error_page(string line, bool &findColon) 
{
	if (line[0] == '/')
	{
		if (ErrorCodesWithoutPage.size() == 0)
			throw runtime_error("no error codes in config for error_page");
		size_t nameLen = line.find_first_of(" \t\f\v\r;#?&%=+\\:");
		if (nameLen == string::npos)
			nameLen = line.length();
		else if (string(" \t\f\v\r;").find(line[nameLen]) == std::string::npos)
			throw runtime_error("invalid character found after error_page");
		string error_page;
		error_page = line.substr(0, nameLen);
		for(uint16_t error_code : ErrorCodesWithoutPage)
		{
			ErrorCodesWithPage.insert({error_code, error_page});
		}
		findColon = true;
		ErrorCodesWithoutPage.clear();
		return (line.substr(nameLen));
	}
	else
	{
		if (line[0] == ';')
			throw runtime_error("syntax error no error_page");
		size_t pos;
		size_t error_num = stoi(line, &pos);
		// if (pos == 0) 
		// 	throw runtime_error("invalid argument entered error_page");
		if (error_num < 300 || error_num > 599)
			throw runtime_error("error code invalid must be between 300 and 599");
		ErrorCodesWithoutPage.push_back(static_cast<uint16_t>(error_num));
		return (line.substr(pos));
	}
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
