#include "Location.hpp"

Location::Location(){
}

Location::Location(const Location &other)  : Aconfig(other)
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
		_upload_store = other._upload_store;
	}
	return (*this);
}

void Location::setPath(string &line)
{
	size_t len = line.find_first_of(" \t\f\v\r{");
	if (len == string::npos)
		len = line.length();
	_path = line.substr(0, len);
	if (Server::directoryCheck(_path) == false)
		throw runtime_error(to_string(_lineNbr) + ": location path: invalid directory path given for location block:" + _path);
	line = line.substr(len);
}

bool Location::checkMethodEnd(bool &findColon, string &line)
{
	static int index = 0;
	const string search[5] = {"{", "deny", "all", ";", "}"};
	if (strncmp(line.c_str(), search[index].c_str(), search[index].length()) == 0)
	{
		if (_methods.size() == 0)
			throw runtime_error(to_string(_lineNbr) + ": limit_except: No methods given for limit_except");
		line = line.substr(search[index].length());
		++index;
		if (index == 5)
		{
			std::cout << line << std::endl;
			findColon = true;
			index = 0;
		}
		return true;
	}
	else if (index != 0)
		throw runtime_error(to_string(_lineNbr) + ": limit_except: couldn't find: " + search[index] + ". after limit_except");
	return false;
}

// bool Location::checkMethodEnd(bool *find, string &line)
// {
// 	enum State
// 	{
// 		OpeningCurly,
// 		Deny,
// 		All,
// 		SemiColon
// 	};
// 	if (find[SemiColon] == true)
// 	{
// 		if (line[0] != '}')
// 			throw runtime_error(to_string(_lineNbr) + ": limit_except: couldn't find closing bracket");
// 		line = line.substr(1);
// 		bzero(find, 4);
// 		return true;
// 	}
// 	else if (find[All] == true)
// 	{
// 		if (line[0] != ';')
// 			throw runtime_error(to_string(_lineNbr) + ": limit_except: couldn't find ; in curly brackets");
// 		find[SemiColon] = true;
// 		line = line.substr(1);
// 		return false;
// 	}
// 	else if (find[Deny] == true)
// 	{
// 		if (strncmp(line.c_str(), "all", 3) != 0)
// 			throw runtime_error(to_string(_lineNbr) + ": limit_except: couldn't find all in curly brackets");
// 		find[All] = true;
// 		line = line.substr(3);
// 		return false;
// 	}
// 	else if (find[OpeningCurly] == true)
// 	{
// 		if (strncmp(line.c_str(), "deny", 4) != 0)
// 			throw runtime_error(to_string(_lineNbr) + ": limit_except: couldn't find deny in curly brackets");
// 		find[Deny] = true;
// 		line = line.substr(4);
// 		return false;
// 	}
// 	else if (line[0] == '{')
// 	{
// 		if (_methods.size() == 0)
// 			throw runtime_error(to_string(_lineNbr) + ": limit_except: No methods given for limit_except");
// 		find[OpeningCurly] = true;
// 		line = line.substr(1);
// 		return false;
// 	}
// 	return false;
// }

bool Location::methods(string &line)
{
	bool findColon = false;
	if (checkMethodEnd(findColon, line) == true)
	{
		if (findColon == true)
		{
			findColon = false;
			return true;
		}
		return false;
	}
	size_t len = line.find_first_of(" \t\f\v\r{");
	if (len == string::npos)
		len = line.length();
	size_t index = (!_methods[0].empty() + !_methods[1].empty() + !_methods[2].empty());
	if (index > 2)
		throw runtime_error(to_string(_lineNbr) + ": limit_except: too many methods given to limit_except");
	_methods[index] = line.substr(0, len);
	std::cout << _methods[index] << std::endl;
	if (strncmp(_methods[index].c_str(), "GET", 3) != 0 && 
	strncmp(_methods[index].c_str(), "POST", 4) != 0 &&
	strncmp(_methods[index].c_str(), "DELETE", 6) != 0)
		throw runtime_error(to_string(_lineNbr) + ": limit_except: Invalid methods given after limit_exept");
	for (ssize_t i = index - 1; i >= 0; --i)
	{
		if (strncmp(_methods[i].c_str(), _methods[index].c_str(), _methods[i].size()) == 0)
			throw runtime_error(to_string(_lineNbr) + ": limit_except: Method given already entered before");
	}
	line = line.substr(len);
	return false;
}

bool Location::indexPage(string &line)
{
	if (line[0] == ';')
	{
		line =  line.substr(1);
		return true;
	}
	size_t len = line.find_first_of(" \t\f\v\r;*?|><:\\");
	if (len == string::npos)
		len = line.length();
	if (string("*?|><:\\").find(line[len]) != string::npos)
		throw runtime_error("invalid character found in filename");
	string indexPage = line.substr(0, len);
	_indexPage.push_back(indexPage);
	line = line.substr(len);
	return false;
}

bool Location::uploadStore(string &line)
{
	if (!_upload_store.empty())
		throw runtime_error(to_string(_lineNbr) + ": upload_store: tried setting second upload_store in block");
	size_t len = line.find_first_of(" \t\f\v\r;");
	if (len == string::npos)
	{
		_upload_store = line;
		return false;
	}
	_upload_store = line.substr(0, len);
	return (handleNearEndOfLine(line, len, "upload_store"));
}

bool Location::extension(string &line)
{
	if (!_cgiExtension.empty())
		throw runtime_error(to_string(_lineNbr) + "extension: tried creating second extension");
	size_t len = line.find_first_of(" \t\f\v\r;");
	if (len == string::npos)
	{
		_cgiExtension = line;
		return false;
	}
	_cgiExtension = line.substr(0, len);
	return (handleNearEndOfLine(line, len, "extension"));
}

bool Location::cgiPath(string &line)
{
	if (!_cgiPath.empty())
		throw runtime_error(to_string(_lineNbr) + "cgi_path: tried creating second cgi_path");
	size_t len = line.find_first_of(" \t\f\v\r;");
	if (len == string::npos)
	{
		_cgiPath = line;
		return false;
	}
	_cgiPath = line.substr(0, len);
	return (handleNearEndOfLine(line, len, "cgi_path"));
}
