#include <ConfigServer.hpp>
#include <utils.hpp>

// Constants for better readability
namespace {
    const char* WHITESPACE_OPEN_BRACKET = " \t\f\v\r{";
    
    // HTTP method bit flags
    const uint8_t HEAD_METHOD_BIT = 1;
    const uint8_t GET_METHOD_BIT = 2;
    const uint8_t POST_METHOD_BIT = 4;
    const uint8_t DELETE_METHOD_BIT = 8;
    const uint8_t ALL_METHODS = HEAD_METHOD_BIT | GET_METHOD_BIT | POST_METHOD_BIT | DELETE_METHOD_BIT;
}

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

void Location::getLocationPath(string &line)
{
	size_t len = line.find_first_of(WHITESPACE_OPEN_BRACKET);
	if (len == string::npos)
		len = line.length();
	_locationPath = line.substr(0, len);
    line = line.substr(len);
    
    // Location paths must start with '/'
	if (_locationPath[0] != '/')
		throw runtime_error(to_string(_lineNbr) + ": location path: invalid location path '" + 
                           _locationPath + "' - must start with '/'");	
}

bool Location::checkMethodEnd(bool &findColon, string &line)
{
	static int expectedTokenIndex = 0;
	const string expectedTokens[5] = {"{", "deny", "all", ";", "}"};
    
	if (strncmp(line.c_str(), expectedTokens[expectedTokenIndex].c_str(), 
                expectedTokens[expectedTokenIndex].length()) == 0)
	{
		if (_allowedMethods == 0)
			throw runtime_error(to_string(_lineNbr) + ": limit_except: No methods specified for limit_except directive");
            
		line = line.substr(expectedTokens[expectedTokenIndex].length());
		++expectedTokenIndex;
        
        // Check if we've found all expected tokens
		if (expectedTokenIndex == 5)
		{
			findColon = true;
			expectedTokenIndex = 0;
		}
		return true;
	}
	else if (expectedTokenIndex != 0)
		throw runtime_error(to_string(_lineNbr) + ": limit_except: expected '" + 
                           expectedTokens[expectedTokenIndex] + "' after limit_except directive");
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
	size_t len = line.find_first_of(WHITESPACE_OPEN_BRACKET);
	if (len == string::npos)
		len = line.length();
    string method = line.substr(0, len);
    uint8_t method_bit;
    if (method == "HEAD")
        method_bit = HEAD_METHOD_BIT;
    else if (method == "GET")
        method_bit = GET_METHOD_BIT;
    else if (method == "POST")
        method_bit = POST_METHOD_BIT;
    else if (method == "DELETE")
        method_bit = DELETE_METHOD_BIT;
    else
        throw runtime_error(to_string(_lineNbr) + ": limit_except: Invalid method given after limit_except: " + method);
    if (_allowedMethods & method_bit)
        throw runtime_error(to_string(_lineNbr) + ": limit_except: Method '" + method + "' already specified");
    _allowedMethods |= method_bit;
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

bool Location::cgiExtension(string &line)
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
    if (_clientMaxBodySize == 0)
        _clientMaxBodySize = curConf.getClientMaxBodySize();
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
        _allowedMethods = ALL_METHODS; // Allow all HTTP methods by default
    }
	for (string &indexPage : _indexPage)
	{
		if (_root[_root.size() - 1] == '/')
			indexPage = _root + indexPage;
		else
			indexPage = _root + "/" + indexPage;
	}
    
    // Set default upload store to root if not specified
	if (_upload_store.empty())
	    _upload_store = _root;
    
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

string Alocation::getUploadStore() const { return _upload_store; }
string Alocation::getExtension() const { return _cgiExtension; }
string Alocation::getCgiPath() const { return _cgiPath; }
string Alocation::getPath() const { return _locationPath; }
uint8_t Alocation::getAllowedMethods() const { return _allowedMethods; }
