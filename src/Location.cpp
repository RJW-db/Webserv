#include "Location.hpp"

Location::Location(){
}

Location::Location(const Location &other)
{
	*this = other;
}

Location Location::operator=(const Location &other)
{
	if (this != &other)
	{
		_path = other._path;
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

string Location::methods(string line, bool &findColon)
{
	size_t len = line.find_first_of(" \t\f\v\r;");
	if (len == string::npos)
	{
		len = line.length();
	}
	
	_methods[_methods.size()] = line.substr(0, len);

}
