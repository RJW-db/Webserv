#include "Location.hpp"

Location::Location(){
}

Location::Location(const Location &other)
{
	*this = other;
}

Location &Location::operator=(const Location &other)
{
	if (this != &other)
	{
		Aconfig::operator=(other);
		_path = other._path;
		_methods = other._methods;
		_indexPage = other._indexPage;
	}
	return (*this);
}

// Location::~Location(){
// }

string Location::setPath(string line)
{
	size_t len = line.find_first_of(" \t\f\v\r{");
	if (len == string::npos)
		len = line.length();
	_path = line.substr(0, len);
	if (Server::directoryCheck(_path) == false)
		throw runtime_error("invalid directory path given for location");
	return (line.substr(len));
}

string Location::methods(string line, bool &findClosingCurly)
{
	enum State
	{
		OpeningCurly,
		Deny,
		All,
		SemiColon,
		total
	};

	static bool find[total] = {0};
	if (find[SemiColon] == true)
	{
		if (line[0] != '}')
			throw runtime_error("couldn't find closing bracket");
		findClosingCurly = true;
		bzero(find, total);
		return (line.substr(1));
	}
	else if (find[All] == true)
	{
		if (line[0] != ';')
			throw runtime_error("couldn't find ; in curly brackets");
		find[SemiColon] = true;
		return (line.substr(1));
	}
	else if (find[Deny] == true)
	{
		if (strncmp(line.c_str(), "all", 3) != 0)
			throw runtime_error("couldn't find all in curly brackets");
		find[All] = true;
		return (line.substr(3));
	}
	else if (find[OpeningCurly] == true)
	{
		if (strncmp(line.c_str(), "deny", 4) != 0)
			throw runtime_error("couldn't find deny in curly brackets");
		find[Deny] = true;
		return (line.substr(4));
	}
	else if (line[0] == '{')
	{
		if (_methods.size() == 0)
			throw runtime_error("No methods given for limit_except");
		find[OpeningCurly] = true;
		return (line.substr(1));
	}
	size_t len = line.find_first_of(" \t\f\v\r{");
	if (len == string::npos)
	{
		len = line.length();
	}
	size_t index = (!_methods[0].empty() + !_methods[1].empty());
	if (index > 1)
		throw runtime_error("Too many methods added");
	_methods[index] = line.substr(0, len);
	if (strncmp(_methods[index].c_str(), "GET", 3) != 0 && 
	strncmp(_methods[index].c_str(), "POST", 4) != 0)
		throw runtime_error("Invalid methods given after limit_exept");
	if (index == 1)
	{
		if (strncmp(_methods[0].c_str(), _methods[index].c_str(), _methods[0].size()) == 0)
			throw runtime_error("Method already entered before");
	}
	return (line.substr(len));
}

string Location::indexPage(string line, bool &findColon)
{
	if (line[0] == ';')
	{
		findColon = true;
		return line.substr(1);
	}
	size_t len = line.find_first_of(" \t\f\v\r;*?|><:\\");
	if (len == string::npos)
		len = line.length();
	if (string("*?|><:\\").find(line[len]) != string::npos)
		throw runtime_error("invalid character found in filename");
	string indexPage = line.substr(0, len);
	// if () // TODO access() to see if file exist/accesible at end of serverblock
	_indexPage.push_back(indexPage);
	return (line.substr(len));
}

