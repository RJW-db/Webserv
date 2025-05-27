#include <Aconfig.hpp>
#include <cstring>

Aconfig::Aconfig(const Aconfig &other)
{
	*this = other;
}


Aconfig &Aconfig::operator=(const Aconfig &other)
{
	if (this != &other)
	{
		ErrorCodesWithPage = other.ErrorCodesWithPage;
		clientBodySize = other.clientBodySize;
		_autoIndex = other._autoIndex;
		_root = other._root;
		_returnRedirect = other._returnRedirect;
	}
	return (*this);
}

string Aconfig::root(string line, bool &findColon)
{
	if (!_root.empty())
		throw runtime_error("Parsing: tried creating second root");
	size_t lenRoot = line.find_first_of(" \t\f\v\r;");
	if (lenRoot == string::npos)
	{
		findColon = false;
		_root = line;
		return line;
	}
	_root = line.substr(0, lenRoot);
	return (handleNearEndOfLine(line, lenRoot, findColon, "root"));
}


string Aconfig::error_page(string line, bool &findColon)
{
	static bool foundPage = false;
	if (foundPage == true && line[0] == ';')
	{
		findColon = true;
		return line.substr(1);
	}
	if (line[0] == '/') // how to check this is last in string
	{
		if (ErrorCodesWithoutPage.size() == 0)
			throw runtime_error("no error codes in config for error_page");
		size_t nameLen = line.find_first_of(" \t\f\v\r;#?&%=+\\:");
		if (nameLen == string::npos)
			nameLen = line.length();
		else if (string(" \t\f\v\r;").find(line[nameLen]) == std::string::npos)
			throw runtime_error("invalid character found after error_page");
		string error_page = line.substr(0, nameLen);
		for(uint16_t error_code : ErrorCodesWithoutPage)
		{
			if (ErrorCodesWithPage.find(error_code) != ErrorCodesWithPage.end())
				ErrorCodesWithPage.at(error_code) = error_page;
			else
				ErrorCodesWithPage.insert({error_code, error_page});
		}
		ErrorCodesWithoutPage.clear();
		foundPage = true;
		return (line.substr(nameLen));
	}
	else
	{
		if (line[0] == ';')
			throw runtime_error("no error page given for error codes");
		if (foundPage == true)
			throw runtime_error("invalid input after error page");
		size_t pos;
		size_t error_num = stoi(line, &pos);
		if (error_num < 300 || error_num > 599)
			throw runtime_error("error code invalid must be between 300 and 599");
		ErrorCodesWithoutPage.push_back(static_cast<uint16_t>(error_num));
		return (line.substr(pos + 1));
	}
}


string Aconfig::ClientMaxBodysize(string line, bool &findColon)
{
	size_t len;

	if (isdigit(line[0]) == false)
		throw runtime_error("Aconfig: first character must be digit");
	clientBodySize = static_cast<size_t>(stoi(line, &len, 10));
	line = line.substr(len);
	if (string("kKmMgG").find(line[0]) != string::npos)
	{
		if (isupper(line[0]) != 0)
			line[0] -= 32;
		if (line[0] == 'g')
			clientBodySize *= 1024;
		if (line[0] == 'g' || line[0] == 'm')
			clientBodySize *= 1024;
		if (line[0] == 'g' || line[0] == 'm' || line[0] == 'k')
			clientBodySize *= 1024;
	}
	else if (string(" \t\f\v\r;").find(line[0]) != string::npos)
		throw runtime_error("Aconfig: invalid character found after value");
	if (line[0] == ';')
	{
		findColon = true;
		return (line.substr(1));
	}
	return handleNearEndOfLine(line, 1, findColon, "ClientMaxBodysize");
}



string Aconfig::indexPage(string line, bool &findColon)
{
	if (line[0] == ';')
	{
		findColon = true;
		return line.substr(1);
	}
	size_t len = line.find_first_not_of(" \t\f\v\r;#?&%=+\\:");
	if (len == 0)
		throw (runtime_error("invalid index given after index"));
	if (len == string::npos)
		len = line.length();
	string indexPage = line.substr(0, len);
	_indexPage.push_back(indexPage);
	return (line.substr(len + 1));
}

string Aconfig::autoIndex(string line, bool &findColon)
{
	size_t len = line.find_first_of(" \t\f\v\r;");
	if (len == string::npos)
		len = line.length();
	string autoIndexing = line.substr(0, len);
	if (strncmp(autoIndexing.c_str(), "on", 2) == 0)
		_autoIndex = true;
	else if (strncmp(autoIndexing.c_str(), "off", 3) == 0)
		_autoIndex = false;
	else
		throw runtime_error("Parsing: expected on/off after autoindex: " + autoIndexing);
	return (handleNearEndOfLine(line, len, findColon, "autoIndex"));
}
#include <iostream>
	// if () // TODO access() to see if file exist/accesible at end of serverblock
string Aconfig::returnRedirect(string line, bool &findColon)
{
	size_t len;
	if (isdigit(line[0]) != 0)
	{
		size_t errorCode = stoi(line, &len);
		if ((errorCode < 301 || errorCode > 303) && (errorCode < 307 || errorCode > 308))
			throw runtime_error("Aconfig: invalid error code given");
		if (!_returnRedirect.second.empty())
			throw runtime_error("Aconfig: cant have multiple return redirects");
		if (_returnRedirect.first != 0)
			throw runtime_error("Aconfig: can't have multiple error code redirects");
		_returnRedirect.first = errorCode;
		return (line.substr(len));
	}
	else
	{
		if (line[0] == ';')
		{
			if (_returnRedirect.first == 0 || _returnRedirect.second.empty())
				throw runtime_error("Aconfig: not enough valid arguments given");
			findColon = true;
			return (line.substr(1));
		}
		len = line.find_first_of(" \t\f\v\r;#?&%=+\\:");
		if (len == 0)
			throw runtime_error("Aconfig: invalid character found");
		if (_returnRedirect.first == 0)
			throw runtime_error("Aconfig: no error code given");
		if (!_returnRedirect.second.empty())
			throw runtime_error("Aconfig: multiple error pages given");
		if (len == string::npos)
			len = line.length();
		_returnRedirect.second = line.substr(0, len);
		return line.substr(len);
	}
}