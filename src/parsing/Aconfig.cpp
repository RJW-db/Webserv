#include <Aconfig.hpp>
#include <Logger.hpp>
#include <unistd.h>

// Constants for better readability
namespace {
    const char* WHITESPACE_SEMICOLON = " \t\f\v\r;";
    const char* WHITESPACE_ONLY = " \t\f\v\r";
    const char* INVALID_PATH_CHARS = " \t\f\v\r;#?&%=+\\:";
    const size_t MIN_ERROR_CODE = 300;
    const size_t MAX_ERROR_CODE = 599;
    const size_t KILOBYTE = 1024;
}

Aconfig::Aconfig() : _autoIndex(autoIndexNotFound), _clientMaxBodySize(0) {}

Aconfig::Aconfig(const Aconfig &other)
{
    *this = other;
}

Aconfig &Aconfig::operator=(const Aconfig &other)
{
    if (this != &other)
    {
        ErrorCodesWithPage = other.ErrorCodesWithPage;
        _clientMaxBodySize = other._clientMaxBodySize;
        _autoIndex = other._autoIndex;
        _root = other._root;
        _returnRedirect = other._returnRedirect;
        _indexPage = other._indexPage;
    }
    return (*this);
}

/**
 * Parse and set the root directory configuration
 * Validates that root starts with '/' and is set only once
 */
bool Aconfig::root(string &line)
{
    if (!_root.empty())
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "tried setting second root");
    if (line[0] != '/')
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "root first character must be /, line: ", line);
    size_t lenRoot = line.find_first_of(WHITESPACE_SEMICOLON);
    if (lenRoot == string::npos)
    {
        _root = line;
        return false;
    }
    _root = line.substr(0, lenRoot);
    return (handleNearEndOfLine(line, lenRoot, "root"));
}

/**
 * Parse error_page directive - handles both error codes and error page paths
 * Format: error_page code1 [code2 ...] /path/to/page;
 */
bool Aconfig::error_page(string &line)
{
    static bool foundPage = false;
    
    // If we've found a page, we expect a semicolon to end the directive
    if (foundPage == true)
    {
        if (line[0] != ';')
            Logger::logExit(ERROR, "Config error at line", _lineNbr, "error_page: invalid input found after error page given");
        foundPage = false;
        line.erase(0, 1);
        return true;
    }
    // If line starts with '/', it's an error page path
    else if (line[0] == '/')
    {
        return setErrorPage(line, foundPage);
    }
    
    // Validate input - shouldn't end without a page
    if (line[0] == ';')
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "error_page: no error page given for error codes");
    if (foundPage == true)
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "error_page: invalid input after error page");
    
    // Parse error code
    size_t pos;
    uint16_t error_num = static_cast<uint16_t>(stoul(line, &pos));
    if (error_num < MIN_ERROR_CODE || error_num > MAX_ERROR_CODE)
        Logger::logExit(ERROR, "Config error at line", _lineNbr,
            "error_page: error code invalid must be between ", MIN_ERROR_CODE, " and ", MAX_ERROR_CODE);

    ErrorCodesWithoutPage.push_back(static_cast<uint16_t>(error_num));
    line.erase(0, pos + 1);
    return false;
}

/**
 * Parse client_max_body_size directive with optional size units (k, m, g)
 * Format: client_max_body_size 1m; (supports k=kilobytes, m=megabytes, g=gigabytes)
 */
bool Aconfig::ClientMaxBodysize(string &line)
{
    size_t len;

    if (isdigit(line[0]) == false)
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "client_max_body_size: first character must be digit: ", line);

    // Parse the numeric value
    _clientMaxBodySize = static_cast<size_t>(stoi(line, &len, 10));
    if (_clientMaxBodySize == 0)
        _clientMaxBodySize = SIZE_MAX;
    line.erase(0, len);
    if (string("kKmMgG;").find(line[0]) != string::npos)
    {
        if (isupper(line[0]) != 0)
            line[0] += 32;
        
        if (line[0] == 'g')
            _clientMaxBodySize *= KILOBYTE;
        if (line[0] == 'g' || line[0] == 'm')
            _clientMaxBodySize *= KILOBYTE;
        if (line[0] == 'g' || line[0] == 'm' || line[0] == 'k')
            _clientMaxBodySize *= KILOBYTE;
    }
    else if (string(WHITESPACE_SEMICOLON).find(line[0]) == string::npos)
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "client_max_body_size: invalid character found after value, line: ", line);
    
    if (line[0] == ';')
    {
        line.erase(0, 1);
        return true;
    }
    return handleNearEndOfLine(line, 1, "client_max_body_size");
}

/**
 * Parse index directive - can specify multiple index files
 * Format: index index.html index.htm default.html;
 */
bool Aconfig::indexPage(string &line)
{
    if (line[0] == ';')
    {
        if (_indexPage.empty())
            Logger::logExit(ERROR, "Config error at line", _lineNbr, ": index: no index given for indexPage, line: ", line);
        line.erase(0, 1);
        return true;
    }
    size_t len = line.find_first_of(INVALID_PATH_CHARS);
    if (len == 0)
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "index: invalid character found, line: ", line);
    if (len == string::npos)
        len = line.length();
    
    string indexPage = line.substr(0, len);
    _indexPage.push_back(indexPage);
    line.erase(0, len + 1);
    return false;
}

/**
 * Parse autoindex directive - enables/disables directory listing
 * Format: autoindex on|off;
 */
bool Aconfig::autoIndex(string &line)
{
    size_t len = line.find_first_of(WHITESPACE_SEMICOLON);
    if (len == string::npos)
        len = line.length();
    
    string autoIndexing = line.substr(0, len);
    if (strncmp(autoIndexing.c_str(), "on", 2) == 0)
        _autoIndex = autoIndexTrue;
    else if (strncmp(autoIndexing.c_str(), "off", 3) == 0)
        _autoIndex = autoIndexFalse;
    else
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "autoIndex: expected on/off after autoindex: ", autoIndexing);
    
    return (handleNearEndOfLine(line, len, "autoIndex"));
}

/**
 * Parse return directive for redirects
 * Format: return 301 http://new-location.com;
 * only handles 301-303 and 307-308
 */
bool Aconfig::returnRedirect(string &line)
{
    size_t len;
    
    if (isdigit(line[0]) != 0)
    {
        uint16_t errorCode = static_cast<uint16_t>(stoul(line, &len));

        if ((errorCode < 301 || errorCode > 303) && (errorCode < 307 || errorCode > 308))
            Logger::logExit(ERROR, "Config error at line", _lineNbr, "Invalid error code in 'return' directive. Line: ", line);
        if (!_returnRedirect.second.empty())
            Logger::logExit(ERROR, "Config error at line", _lineNbr, "Multiple return redirects are not allowed in 'return' directive. Line: ", line);
        if (_returnRedirect.first != 0)
            Logger::logExit(ERROR, "Config error at line", _lineNbr, "Multiple error code redirects are not allowed in 'return' directive. Line: ", line);
        _returnRedirect.first = errorCode;
        line.erase(0, len);
        return false;
    }
    else
    {
        if (line[0] == ';')
        {
            if (_returnRedirect.first == 0 || _returnRedirect.second.empty())
                Logger::logExit(ERROR, "Config error at line", _lineNbr, "Not enough valid arguments for 'return' directive. Line: ", line);
            line.erase(0, 1);
            return true;
        }
        
        len = line.find_first_of(WHITESPACE_SEMICOLON);
        if (len == 0)
            Logger::logExit(ERROR, "Config error at line", _lineNbr, "Invalid character found in 'return' directive. Line: ", line);
        if (_returnRedirect.first == 0)
            Logger::logExit(ERROR, "Config error at line", _lineNbr, "No error code given in 'return' directive. Line: ", line);
        if (!_returnRedirect.second.empty())
            Logger::logExit(ERROR, "Config error at line", _lineNbr, "Multiple error pages specified in 'return' directive. Line: ", line);

        if (len == string::npos)
            len = line.length();
        
        _returnRedirect.second = line.substr(0, len);
        line.erase(0, len);
        return false;
    }
}

/**
 * Set the current line number for error reporting
 */
void Aconfig::setLineNbr(int num)
{
    _lineNbr = num;
}

/**
 * Helper function to set error page for all pending error codes
 * Validates path and assigns it to all error codes in the queue
 */
bool Aconfig::setErrorPage(string &line, bool &foundPage)
{
    // Ensure we have error codes waiting for a page assignment
    if (ErrorCodesWithoutPage.size() == 0)
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "error_page: no error codes in config for error_page");
    
    // Find the end of the error page path
    size_t nameLen = line.find_first_of(INVALID_PATH_CHARS);
    if (nameLen == string::npos)
        nameLen = line.length();
    else if (string(WHITESPACE_SEMICOLON).find(line[nameLen]) == std::string::npos)
        Logger::logExit(ERROR, "Config error at line", _lineNbr, "error_page: invalid character found after error_page");

    string error_page = line.substr(0, nameLen);
    
    // Assign the error page to all pending error codes
    for (uint16_t error_code : ErrorCodesWithoutPage)
    {
        if (ErrorCodesWithPage.find(error_code) != ErrorCodesWithPage.end())
            ErrorCodesWithPage.at(error_code) = error_page;
        else
            ErrorCodesWithPage.insert({error_code, error_page});
    }
    
    ErrorCodesWithoutPage.clear();
    foundPage = true;
    line.erase(0, nameLen);
    return false;
}

/**
 * Handle whitespace and semicolon at the end of configuration lines
 * Returns true if a semicolon was found (end of directive)
 */
bool Aconfig::handleNearEndOfLine(string &line, size_t pos, string err)
{
    size_t index = line.find_first_not_of(WHITESPACE_ONLY, pos);
    if (index == string::npos)
        return false;
    if (line[index] != ';')
        Logger::logExit(ERROR, "Config error at line", _lineNbr, err, ": invalid input found before semi colon: ", line);
    line.erase(0, index + 1);
    return true;
}

/**
 * Set default error pages for common HTTP error codes
 * Creates mappings for standard error codes to default HTML files
 */
void Aconfig::setDefaultErrorPages()
{
    uint16_t errorCodes[] = {400, 403, 404, 405, 413, 414, 431, 500, 501, 502, 503, 504};
    size_t numErrorCodes = sizeof(errorCodes) / sizeof(errorCodes[0]);

    for (size_t i = 0; i < numErrorCodes; i++)
    {
        uint16_t errorCode = errorCodes[i];
        
        if (ErrorCodesWithPage.find(errorCode) == ErrorCodesWithPage.end())
        {
            // Use default error page
            string fileName = "./webPages/defaultErrorPages/" + to_string(static_cast<int>(errorCode)) + ".html";
            ErrorCodesWithPage.insert({errorCode, fileName});
        }
        else
        {
            // Prepend root to custom error page path
            ErrorCodesWithPage.at(errorCode).insert(0, _root);
            ErrorCodesWithPage.at(errorCode).erase(0, 1);
            if (ErrorCodesWithPage.at(errorCode)[0] == '/')
                ErrorCodesWithPage.at(errorCode).erase(0, 1);
        }
        
        // Verify the error page file is accessible
        if (access(ErrorCodesWithPage.at(errorCode).c_str(), R_OK) != 0)
            Logger::logExit(ERROR, "Config error at line", _lineNbr, "couldn't open error page: ", ErrorCodesWithPage.at(errorCode));
    }
}

size_t Aconfig::getClientMaxBodySize() const { return _clientMaxBodySize; }
string Aconfig::getRoot() const { return _root; }
int8_t Aconfig::getAutoIndex() const { return _autoIndex; }
pair<uint16_t, string> Aconfig::getReturnRedirect() const { return _returnRedirect; }
map<uint16_t, string> Aconfig::getErrorCodesWithPage() const { return ErrorCodesWithPage; }
vector<string> Aconfig::getIndexPage() const { return _indexPage; }
