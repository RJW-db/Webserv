#include <ConfigServer.hpp>
#include <utils.hpp>



// Location::Location(string &path)
// {
// 	_path = path;
//     std::cout << "Path:" << path << std::endl;
// }

Location::Location(const Location &other) : Alocation(other)
{
	*this = other;
}

Location &Location::operator=(const Location &other)
{
	if (this != &other)
	{
		Alocation::operator=(other);
	}
	return (*this);
}

string Location::getLocationPath(string &line)
{
	size_t len = line.find_first_of(" \t\f\v\r{");
	if (len == string::npos)
		len = line.length();
	string path = line.substr(0, len);
	if (path[0] != '/')
		throw runtime_error(to_string(_lineNbr) + ": location path: invalid location path given for location block: " + path);	
	// string pathCheck = path.substr(1);
	// if (path.length() != 1 && directoryCheck(pathCheck) == false)
	// 	throw runtime_error(to_string(_lineNbr) + ": location path: invalid directory path given for location block:" + path);
	line = line.substr(len);
	_locationPath = path;
	return path;
}

bool Location::checkMethodEnd(bool &findColon, string &line)
{
	static int index = 0;
	const string search[5] = {"{", "deny", "all", ";", "}"};
	if (strncmp(line.c_str(), search[index].c_str(), search[index].length()) == 0)
	{
		if (_allowedMethods == 0)
			throw runtime_error(to_string(_lineNbr) + ": limit_except: No methods given for limit_except");
		line = line.substr(search[index].length());
		++index;
		if (index == 5)
		{
			findColon = true;
			index = 0;
		}
		return true;
	}
	else if (index != 0)
		throw runtime_error(to_string(_lineNbr) + ": limit_except: couldn't find: " + search[index] + ". after limit_except");
	return false;
}

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
    string method = line.substr(0, len);
    uint8_t binary_bit;
    if (method == "HEAD")
        binary_bit = 1;
    else if (method == "GET")
        binary_bit = 2;
    else if (method == "POST")
        binary_bit = 4;
    else if (method == "DELETE")
        binary_bit = 8;
    else
        throw runtime_error(to_string(_lineNbr) + ": limit_except: Invalid methods given after limit_exept");
    if (_allowedMethods & binary_bit)
        throw runtime_error(to_string(_lineNbr) + ": limit_except: Method given already entered before");
    _allowedMethods += binary_bit;
	line = line.substr(len);
	return false;
}

// bool Location::methods(string &line)
// {
// 	bool findColon = false;
// 	if (checkMethodEnd(findColon, line) == true)
// 	{
// 		if (findColon == true)
// 		{
// 			findColon = false;
// 			return true;
// 		}
// 		return false;
// 	}
// 	size_t len = line.find_first_of(" \t\f\v\r{");
// 	if (len == string::npos)
// 		len = line.length();
// 	size_t index = (!_methods[0].empty() + !_methods[1].empty() + !_methods[2].empty());
// 	if (index > 2)
// 		throw runtime_error(to_string(_lineNbr) + ": limit_except: too many methods given to limit_except");
// 	_methods[index] = line.substr(0, len);
// 	if (strncmp(_methods[index].c_str(), "GET", 3) != 0 && 
// 	strncmp(_methods[index].c_str(), "POST", 4) != 0 &&
// 	strncmp(_methods[index].c_str(), "DELETE", 6) != 0)
// 		throw runtime_error(to_string(_lineNbr) + ": limit_except: Invalid methods given after limit_exept");
// 	for (ssize_t i = index - 1; i >= 0; --i)
// 	{
// 		if (strncmp(_methods[i].c_str(), _methods[index].c_str(), _methods[i].size()) == 0)
// 			throw runtime_error(to_string(_lineNbr) + ": limit_except: Method given already entered before");
// 	}
// 	line = line.substr(len);
// 	return false;
// }

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
		throw runtime_error(to_string(_lineNbr) + ": upload_store: setting second upload_store in block");
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


void Location::SetDefaultLocation(Aconfig &curConf)
{
    if (_autoIndex == autoIndexNotFound)
        _autoIndex = curConf.getAutoIndex();
    if (_root.empty())
        _root = curConf.getRoot();
	else 
		_root.insert(0, ".");
	if (_root[_root.size() - 1] == '/' && _root.size() > 2)
		_root = _root.substr(0, _root.size() - 1);
    if (_locationPath[_locationPath.size() - 1] == '/' && _locationPath.size() > 1)
        _locationPath = _locationPath.substr(0, _locationPath.size() - 1);
    if (_root.size() > 2)
        _locationPath = _root + _locationPath;
	else
        _locationPath = "." + _locationPath;
    if (_clientBodySize == 0)
        _clientBodySize = curConf.getClientBodySize();
    if (_returnRedirect.first == 0)
        _returnRedirect = curConf.getReturnRedirect();
    for (pair<uint16_t, string> errorCodePages : curConf.getErrorCodesWithPage())
        ErrorCodesWithPage.insert(errorCodePages);
    if (_indexPage.empty())
    {
        for (string indexPage : curConf.getIndexPage())
            _indexPage.push_back(indexPage);
    }
    if (_allowedMethods == 0)
    {
        _allowedMethods = 1 + 2 + 4 + 8;
    }
	for (string &indexPage : _indexPage)
	{
		if (_root[_root.size() - 1] == '/')
			indexPage = _root + indexPage;
		else
			indexPage = _root + "/" + indexPage;
	}
    // if (_locationPath.size() > 2 && directoryCheck(_locationPath) == false)
    //     throw runtime_error("invalid root + path given: " + _locationPath);
	if (_upload_store.empty())
	    _upload_store = _root;
    else
    {
        
    }
    setDefaultErrorPages();
}


Alocation::Alocation(const Alocation &other)
: Aconfig(other)
{
    *this = other;
}

Alocation &Alocation::operator=(const Alocation &other)
{
    if (this != &other)
	{
        Aconfig::operator=(other);
		_upload_store = other._upload_store;
		_cgiPath = other._cgiPath;
		_cgiExtension = other._cgiExtension;
        _locationPath = other._locationPath;
        _allowedMethods = other._allowedMethods;
	}
	return (*this);
}

string Alocation::getUploadStore() const
{
	return _upload_store;
}
string Alocation::getExtension() const
{
	return _cgiExtension;
}
string Alocation::getCgiPath() const
{
	return _cgiPath;
}

string Alocation::getPath() const
{
	return _locationPath;
}

uint8_t Alocation::getAllowedMethods() const
{
    return _allowedMethods;
}