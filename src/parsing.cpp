#include <Parsing.hpp>
#include <ConfigServer.hpp>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <Location.hpp>

using namespace std;

// Constants for better readability
namespace {
    const char* WHITESPACE_CHARS = " \t\f\v\r";
    const char* WHITESPACE_WITH_NEWLINE = " \t\f\v\r\n";
    const size_t SERVER_KEYWORD_LENGTH = 6;
    const size_t LOCATION_KEYWORD_LENGTH = 8;
}

/**
 * Checks if a line is empty or contains only whitespace/comments
 * @param line The line to check
 * @param skipSpace Output parameter for the position of first non-whitespace character
 * @return true if line is empty or comment-only, false otherwise
 */
bool isEmptyOrCommentLine(string &line, size_t &skipSpace)
{
    skipSpace = line.find_first_not_of(WHITESPACE_CHARS);
    if (string::npos == skipSpace || line[skipSpace] == '#')
    {
        return true;
    }
    return false;
}

/**
 * Removes leading whitespace from a string
 * @param line The string to trim (modified in place)
 */
void trimLeadingWhitespace(string &line)
{
    size_t skipSpace = line.find_first_not_of(WHITESPACE_CHARS);
    if (skipSpace != 0 && skipSpace != string::npos)
        line = line.substr(skipSpace);
}

/**
 * Validates that there is whitespace after a command
 * @param line The line to check
 * @param whitespaceChars The whitespace characters to check for
 * @param lineNumber The line number for error reporting
 * @throws runtime_error if no whitespace found
 */
void validateWhitespaceAfterCommand(const string& line, const char* whitespaceChars, int lineNumber = 0)
{
    if (line.empty() || string(whitespaceChars).find(line[0]) == std::string::npos)
    {
        string errorMsg = "no space found after command";
        if (lineNumber > 0)
            errorMsg = to_string(lineNumber) + ": " + errorMsg;
        throw runtime_error(errorMsg);
    }
}

/**
 * Checks if line contains a closing bracket and handles it
 * @return false if closing bracket found, true otherwise
 */
bool    Parsing::runReadblock()
{
    size_t  skipSpace;
	if (validSyntax == false)
		throw runtime_error(to_string(_lines.begin()->first) + ": invalid syntax: " + _lines.begin()->second);
    
    if (_lines.begin()->second[0] == '}')
    {
        _lines.begin()->second =  _lines.begin()->second.substr(1);
        if (isEmptyOrCommentLine(_lines.begin()->second, skipSpace) == true)
            _lines.erase(_lines.begin());
        return false;
    }
    return true;
}


/**
 * checks if line should be removed and set to next line
 * @param line output paramater to set to newline if should be skipped
 * @param forceSkip for forcing rest of current line to be skipped and removed
 * @param curConf Config to set new lineNbr
 * @param shouldSkipSpace for setting if trimleadingWhiteSpace should be run
 * 
 */
template <typename T>
void Parsing::skipLine(string &line, bool forceSkip, T &curConf, bool shouldSkipSpace)
{
	size_t skipSpace = line.find_first_not_of(WHITESPACE_CHARS);
    if (string::npos == skipSpace || forceSkip)
    {
        if (_lines.size() <= 1)
            throw runtime_error("No closing bracket found after block: " + to_string(_lines.begin()->first));
        _lines.erase(_lines.begin());
        line = _lines.begin()->second;
        curConf.setLineNbr(_lines.begin()->first);
    }
    if (shouldSkipSpace == true)
        trimLeadingWhitespace(line);
}

/**
 * adds location block to configServer
 * @param line current line being used for cmd
 * @param block current ConfigServerblock used to add new location block to
 * @param cmd Function to run using line
 */
template <typename T>
void Parsing::LocationCheck(string &line, T &block, bool &validSyntax)
{
    if constexpr (std::is_same<T, ConfigServer>::value) // checks if block == Configserver
    {
        Location location;
        location.setLineNbr(_lines.begin()->first);
        line = line.substr(LOCATION_KEYWORD_LENGTH);
        validateWhitespaceAfterCommand(line, WHITESPACE_CHARS, _lines.begin()->first);
        skipLine(line, false, block, true);
        location.getLocationPath(line);
        skipLine(line, false, block, true);
        if (line[0] != '{')
            throw runtime_error(to_string(_lines.begin()->first) + ": couldn't find opening curly bracket for location");
        line = line.substr(1);
        skipLine(line, false, block, false);
        const map<string, bool (Location::*)(string &)> cmds = {
            {"root", &Location::root},
            {"client_max_body_size", &Location::ClientMaxBodysize},
            {"autoindex", &Location::autoIndex},
            {"upload_store", &Location::uploadStore}};
        const map<string, bool (Location::*)(string &)> whileCmds = {
            {"error_page", &Location::error_page},
            {"limit_except", &Location::methods},
            {"return", &Location::returnRedirect},
            {"index", &Location::indexPage}};
		_lines.begin()->second = line;
		readBlock(location, cmds, whileCmds);
		line = _lines.begin()->second;
        block.addLocation(location, location.getPath());
        validSyntax = true;
    }
    else
        throw runtime_error(to_string(_lines.begin()->first) + ": location block can only be used in server block");
}

/**
 * runs cmd to store for configserver or location
 * @param line current line being used for cmd
 * @param block current ConfigServer Or location block used to run and store cmd
 * @param cmd Function to run using line
 */
template <typename T>
void Parsing::whileCmdCheck(string &line, T &block, const pair<const string, bool (T::*)(string &)> &cmd)
{
	bool foundSemicolon;
	line = line.substr(cmd.first.size());
	validateWhitespaceAfterCommand(line, WHITESPACE_WITH_NEWLINE, _lines.begin()->first);
	
	size_t argumentCount = 0;
	while (++argumentCount)
	{
		skipLine(line, false, block, true);
		foundSemicolon = (block.*(cmd.second))(line);
		skipLine(line, false, block, true);
		if (foundSemicolon == true)
		{
			if (argumentCount == 1)
				throw runtime_error(string("no arguments after ") + cmd.first);
			break;
		}
	}
}

/**
 * runs cmd to store for configserver or location
 * @param line current line being used for cmd
 * @param block current ConfigServer Or location block used to run and store cmd
 * @param cmd Function to run using line
 */
template <typename T>
void Parsing::cmdCheck(string &line, T &block, const pair<const string, bool (T::*)(string &)> &cmd)
{
	bool foundSemicolon;
    line = line.substr(cmd.first.size());
    skipLine(line, false, block, false);
    validateWhitespaceAfterCommand(line, WHITESPACE_CHARS, _lines.begin()->first);
    trimLeadingWhitespace(line);
    foundSemicolon = (block.*(cmd.second))(line);
    if (!foundSemicolon)
    {
        skipLine(line, true, block, false);
        if (line[0] != ';')
            throw runtime_error(to_string(_lines.begin()->first) + ": no semi colon found after input");
        line = line.substr(1);
    }
    skipLine(line, false, block, false);
}

/**
 * stores commands for location and configServer
 */
template <typename T>
void Parsing::readBlock(T &block, 
    const map<string, bool (T::*)(string &)> &cmds, 
    const map<string, bool (T::*)(string &)> &whileCmds)
{
    string line = _lines.begin()->second;
    do
    {
		validSyntax = false;
        trimLeadingWhitespace(line);
        for (const auto& cmd : cmds)
        {
            if (strncmp(line.c_str(), cmd.first.c_str(), cmd.first.size()) == 0)
			{
    			validSyntax = true;
                cmdCheck(line, block, cmd);
			}
        }
		if (validSyntax == false)
		{
			for (const auto& whileCmd : whileCmds)
			{
				if (strncmp(line.c_str(), whileCmd.first.c_str(), whileCmd.first.size()) == 0)
				{
					whileCmdCheck(line, block, whileCmd);
					validSyntax = true;
				}
			}
		}
        if (strncmp(line.c_str(), "location", LOCATION_KEYWORD_LENGTH) == 0 && validSyntax == false)
            LocationCheck(line, block, validSyntax);
		if (validSyntax == false)
			std::cout << "line:" << line << ":" << _lines.begin()->second << std::endl;
    } 
	while (runReadblock() == true);
}

/**
 * sets configserver and starts readblock for server
 */
void Parsing::ServerCheck()
{
    ConfigServer curConf;
    curConf.setLineNbr(_lines.begin()->first);
	string line = _lines.begin()->second;
    line = line.substr(SERVER_KEYWORD_LENGTH);
    skipLine(line, false, curConf, true);
    if (line[0] == '{')
    {
        line = line.substr(1);
        skipLine(line, false, curConf, false);
        const std::map<string, bool (ConfigServer::*)(string &)> cmds = {
            {"listen", &ConfigServer::listenHostname},
            {"root", &ConfigServer::root},
            {"client_max_body_size", &ConfigServer::ClientMaxBodysize},
            {"server_name", &ConfigServer::serverName},
            {"autoindex", &Location::autoIndex}};
        const map<string, bool (ConfigServer::*)(string &)> whileCmds = {
            {"error_page", &ConfigServer::error_page},
            {"return", &ConfigServer::returnRedirect}};
		_lines.begin()->second = line;
        readBlock(curConf, cmds, whileCmds);
        curConf.setDefaultConf();
        _configs.push_back(curConf);
    }
    else
    {
        
        throw runtime_error(to_string(curConf.getLineNbr()) + ": Couldn't find opening curly bracket server block");
    }
}

/**
 * Reads and and stores lines in config file while removing the comments and leading whitespace
 * @param input Path to the configuration file
 */
void Parsing::readConfigFile(const char *input)
{
    fstream fs;
    fs.open(input, fstream::in);
    if (fs.is_open() == false)
	{
		std::cout << input << std::endl;
		std::cout << "Current directory: " << getcwd(NULL, 0) << std::endl;
		throw runtime_error("inputfile couldn't be opened: " + string(input));
	}
    
    string line;
    size_t skipSpace;
    size_t lineNbr = 0;
    while (getline(fs, line))
    {
        ++lineNbr;
        if (isEmptyOrCommentLine(line, skipSpace) == true)
            continue; // Empty line, or #(comment)
        line = line.substr(skipSpace);
        size_t commentIndex = line.find('#');
        if (commentIndex != string::npos)
            line = line.substr(0, commentIndex); // remove comments after text
        _lines.insert({lineNbr, line});
    }
    fs.close();
}

/**
 * Processes all server blocks in the configuration
 */
void Parsing::processServerBlocks()
{
    while (!_lines.empty())
    {
        if (strncmp(_lines.begin()->second.c_str(), "server", SERVER_KEYWORD_LENGTH) == 0)
            ServerCheck();
        else
            throw runtime_error(to_string(_lines.begin()->first) + "Invalid line found expecting server");
    }
}

Parsing::Parsing(const char *input)
{
    readConfigFile(input);
    processServerBlocks();
}

Parsing::~Parsing()
{
    
}

/**
 * prints all values stored in servers and locations
 */
void Parsing::printAll() const
{
	for (ConfigServer config : _configs)
	{
		cout << "Server Name: " << config.getServerName() << endl;
		cout << "Port and Host:" << endl;
		for (const auto &portHost : config.getPortHost())
		{
			cout << "  Port: " << portHost.first << endl << ", Host: " << portHost.second << endl;
		}
			cout << "  Root: " << config.getRoot() << endl;
			cout << "  Client Max Body Size: " << config.getClientMaxBodySize() << endl;
			cout << "  Auto Index: " << (config.getAutoIndex() == autoIndexTrue ? "True" : "False") << endl;
			
			// cout << "  Error Pages: ";
			// for (const auto &errorPage : config.getErrorCodesWithPage())
			// {
			// 	cout << errorPage.first << " -> " << errorPage.second << ", ";
			// }
			// cout << endl;
			
			cout << "  Return Redirect: " << config.getReturnRedirect().first << " -> " << config.getReturnRedirect().second << endl;
			cout << "  Index Pages: ";
			for (const string &index : config.getIndexPage())
			{
				cout << index << " ";
			}
			cout << endl;
		for (auto pair : config.getLocations())
		{
			Alocation &location = pair.second;
			cout << "location: " << "Path: " << pair.first << endl;
			cout << "  Root: " << location.getRoot() << endl;
			cout << "  Client Max Body Size: " << location.getClientMaxBodySize() << endl;
			cout << "  Auto Index: " << (location.getAutoIndex() == autoIndexTrue ? "True" : "False") << endl;
			
			// cout << "  Error Pages: ";
			// for (const auto &errorPage : location.getErrorCodesWithPage())
			// {
			// 	cout << errorPage.first << " -> " << errorPage.second << ", ";
			// }
			// cout << endl;
			
			cout << "  Return Redirect: " << location.getReturnRedirect().first << " -> " << location.getReturnRedirect().second << endl;
			cout << "  Index Pages: ";
			for (const string &index : location.getIndexPage())
			{
				cout << index << " ";
			}
			cout << endl;
			cout << "  Methods: ";
			if (location.getAllowedMethods() & 1)
                cout << "HEAD ";
            if (location.getAllowedMethods() & 2)
                cout << "GET ";
            if (location.getAllowedMethods() & 4)
                cout << "POST ";
            if (location.getAllowedMethods() & 8)
                cout << "DELETE ";
                
			cout << endl;
			cout << "  Upload Store: " << location.getUploadStore() << endl;
			cout << "  CGI Extension: " << location.getExtension() << endl;
			cout << "  CGI Path: " << location.getCgiPath() << endl;
		}
	}
}

vector<ConfigServer> &Parsing::getConfigs()
{
	return _configs;
}