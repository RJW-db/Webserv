#include <Aconfig.hpp>
#include <cstring>

Aconfig::Aconfig() : _autoIndex(autoIndexNotFound),  _clientBodySize(0) {}

Aconfig::Aconfig(const Aconfig &other)
{
	*this = other;
}


Aconfig &Aconfig::operator=(const Aconfig &other)
{
	if (this != &other)
	{
		ErrorCodesWithPage = other.ErrorCodesWithPage;
		_clientBodySize = other._clientBodySize;
		_autoIndex = other._autoIndex;
		_root = other._root;
		_returnRedirect = other._returnRedirect;
		_indexPage = other._indexPage;
	}
	return (*this);
}

bool Aconfig::root(string &line)
{
	if (!_root.empty())
		throw runtime_error(to_string(_lineNbr) + ": root: tried setting second root");
	if (line[0] != '/')
		throw runtime_error(to_string(_lineNbr) + ": root: first character must be /: " + line);
	size_t lenRoot = line.find_first_of(" \t\f\v\r;");
	if (lenRoot == string::npos)
	{
		_root = line;
		return false;
	}
	_root = line.substr(0, lenRoot);
	return (handleNearEndOfLine(line, lenRoot, "root"));
}

bool Aconfig::setErrorPage(string &line, bool &foundPage)
{
	if (ErrorCodesWithoutPage.size() == 0)
		throw runtime_error(to_string(_lineNbr) + ": error_page: no error codes in config for error_page");
	size_t nameLen = line.find_first_of(" \t\f\v\r;#?&%=+\\:");
	if (nameLen == string::npos)
		nameLen = line.length();
	else if (string(" \t\f\v\r;").find(line[nameLen]) == std::string::npos)
		throw runtime_error(to_string(_lineNbr) + ": error_page: invalid character found after error_page");
	string error_page = line.substr(0, nameLen);
	for (uint16_t error_code : ErrorCodesWithoutPage)
	{
		if (ErrorCodesWithPage.find(error_code) != ErrorCodesWithPage.end())
			ErrorCodesWithPage.at(error_code) = error_page;
		else
			ErrorCodesWithPage.insert({error_code, error_page});
	}
	ErrorCodesWithoutPage.clear();
	foundPage = true;
	line = line.substr(nameLen);
	return false;
}

#include <iostream>
bool Aconfig::error_page(string &line)
{
	static bool foundPage = false;
	if (foundPage == true)
	{
		if (line[0] != ';')
			throw runtime_error(to_string(_lineNbr) + ": error_page: invalid input found after error page given");
		foundPage = false;
		line = line.substr(1);
		return true;
	}
	else if (line[0] == '/')
	{
		return setErrorPage(line, foundPage);
	}
	if (line[0] == ';')
		throw runtime_error(to_string(_lineNbr) + ": error_page: no error page given for error codes");
	if (foundPage == true)
		throw runtime_error(to_string(_lineNbr) + ": error_page: invalid input after error page");
	size_t pos;
	size_t error_num = stoi(line, &pos);
	if (error_num < 300 || error_num > 599)
		throw runtime_error(to_string(_lineNbr) + ": error_page: error code invalid must be between 300 and 599");
	ErrorCodesWithoutPage.push_back(static_cast<uint16_t>(error_num));
	line = line.substr(pos + 1);
	return false;
}

bool Aconfig::ClientMaxBodysize(string &line)
{
	size_t len;

	if (isdigit(line[0]) == false)
		throw runtime_error(to_string(_lineNbr) + ": client_max_body_size: first character must be digit: " + line[0]);
	_clientBodySize = static_cast<size_t>(stoi(line, &len, 10));
	if (_clientBodySize == 0)
		_clientBodySize = SIZE_MAX;
	line = line.substr(len);
	if (string("kKmMgG").find(line[0]) != string::npos)
	{
		if (isupper(line[0]) != 0)
			line[0] += 32;
		if (line[0] == 'g')
			_clientBodySize *= 1024;
		if (line[0] == 'g' || line[0] == 'm')
			_clientBodySize *= 1024;
		if (line[0] == 'g' || line[0] == 'm' || line[0] == 'k')
			_clientBodySize *= 1024;
	}
	else if (string(" \t\f\v\r;").find(line[0]) == string::npos)
		throw runtime_error(to_string(_lineNbr) + ": client_max_body_size: invalid character found after value: " + line[0]);
	return handleNearEndOfLine(line, 1, "client_max_body_size");
}

bool Aconfig::indexPage(string &line)
{
	if (line[0] == ';')
	{
		if (_indexPage.empty())
			throw runtime_error(to_string(_lineNbr) + ": index: no index given for indexPage");
		line = line.substr(1);
		return true;
	}
	size_t len = line.find_first_not_of(" \t\f\v\r;#?&%=+\\:");
	if (len == 0)
		throw (runtime_error(to_string(_lineNbr) + ": index: invalid index given after index"));
	if (len == string::npos)
		len = line.length();
	string indexPage = line.substr(0, len);
	_indexPage.push_back(indexPage);
	line = line.substr(len + 1);
	return false;
}

bool Aconfig::autoIndex(string &line)
{
	size_t len = line.find_first_of(" \t\f\v\r;");
	if (len == string::npos)
		len = line.length();
	string autoIndexing = line.substr(0, len);
	if (strncmp(autoIndexing.c_str(), "on", 2) == 0)
		_autoIndex = autoIndexTrue;
	else if (strncmp(autoIndexing.c_str(), "off", 3) == 0)
		_autoIndex = autoIndexFalse;
	else
		throw runtime_error(to_string(_lineNbr) + "autoIndex: expected on/off after autoindex: " + autoIndexing);
	return (handleNearEndOfLine(line, len, "autoIndex"));
}

bool Aconfig::returnRedirect(string &line)
{
	size_t len;
	if (isdigit(line[0]) != 0)
	{
		size_t errorCode = stoi(line, &len);
		if ((errorCode < 301 || errorCode > 303) && (errorCode < 307 || errorCode > 308))
			throw runtime_error(to_string(_lineNbr) + ": return: invalid error code given");
		if (!_returnRedirect.second.empty())
			throw runtime_error(to_string(_lineNbr) + ": return: cant have multiple return redirects");
		if (_returnRedirect.first != 0)
			throw runtime_error(to_string(_lineNbr) + ": return: can't have multiple error code redirects");
		_returnRedirect.first = errorCode;
		line = line.substr(len);
		return false;
	}
	else
	{
		if (line[0] == ';')
		{
			if (_returnRedirect.first == 0 || _returnRedirect.second.empty())
				throw runtime_error(to_string(_lineNbr) + ": return: not enough valid arguments given");
			line = line.substr(1);
			return true;
		}
		len = line.find_first_of(" \t\f\v\r;#?&%=+\\:");
		if (len == 0)
			throw runtime_error(to_string(_lineNbr) + ": return: invalid character found");
		if (_returnRedirect.first == 0)
			throw runtime_error(to_string(_lineNbr) + ": return: no error code given");
		if (!_returnRedirect.second.empty())
			throw runtime_error(to_string(_lineNbr) + ": return: multiple error pages given");
		if (len == string::npos)
			len = line.length();
		_returnRedirect.second = line.substr(0, len);
		line = line.substr(len);
		return false;
	}
}

void Aconfig::setLineNbr(int num)
{
	_lineNbr = num;
}

bool Aconfig::handleNearEndOfLine(string &line, size_t pos, string err)
{
	size_t k = line.find_first_not_of(" \t\f\v\r", pos);
	if (k == string::npos)
	{
		return false;
	}
	if (line[k] != ';')
	{
		throw runtime_error(to_string(_lineNbr) + ": " + err + ": invalid input found before semi colon: " + line[k]);
	}
	line = line.substr(k + 1);
	return true;
}

void Aconfig::setDefaultErrorPages()
{
    uint16_t errorCodes[12] = {400, 403, 404, 405, 413, 414, 431, 500, 501, 502, 503, 504};

    for (uint16_t errorCode : errorCodes)
    {
        if (ErrorCodesWithPage.find(errorCode) == ErrorCodesWithPage.end())
        {
            string fileName = "./webPages/defaultErrorPages/" + to_string(static_cast<int>(errorCode)) + ".html";
            ErrorCodesWithPage.insert({errorCode, fileName});
        }
        else
        {
            ErrorCodesWithPage.at(errorCode).insert(0, _root);
            ErrorCodesWithPage.at(errorCode) = ErrorCodesWithPage.at(errorCode).substr(1);
            if (ErrorCodesWithPage.at(errorCode)[0] == '/')
                ErrorCodesWithPage.at(errorCode) = ErrorCodesWithPage.at(errorCode).substr(1);
        }
        if (access(ErrorCodesWithPage.at(errorCode).c_str(), R_OK) != 0)
            throw runtime_error("couldn't open error page:" + ErrorCodesWithPage.at(errorCode));
    }
}

size_t Aconfig::getClientBodySize() const
{
	return _clientBodySize;
}
string Aconfig::getRoot() const
{
	return _root;
}
int8_t Aconfig::getAutoIndex() const
{
	return _autoIndex;
}
pair<uint16_t, string> Aconfig::getReturnRedirect() const
{
	return _returnRedirect;
}
map<uint16_t, string> Aconfig::getErrorCodesWithPage() const
{
	return ErrorCodesWithPage;
}
vector<string> Aconfig::getIndexPage() const
{
	return _indexPage;
}
