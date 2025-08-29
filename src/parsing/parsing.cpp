#include "ConfigServer.hpp"
#include "Parsing.hpp"
#include "Logger.hpp"
using namespace std;
namespace
{
    constexpr const char *WHITESPACE_CHARS = " \t\f\v\r";
    constexpr const char *WHITESPACE_WITH_NEWLINE = " \t\f\v\r\n";
    constexpr size_t SERVER_KEYWORD_LENGTH = 6;
    constexpr size_t LOCATION_KEYWORD_LENGTH = 8;

    bool isEmptyOrCommentLine(string &line, size_t &skipSpace);
    void trimLeadingWhitespace(string &line);
    void validateWhitespaceAfterCommand(const string &line, const char *validChars, int lineNbr);
}

Parsing::Parsing(const char *input)
{
    readConfigFile(input);
    processServerBlocks();
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
        Logger::logExit(ERROR, "Config error", '-', "Failed to open ", input);

    string line;
    size_t skipSpace;
    size_t lineNbr = 0;
    while (getline(fs, line))
    {
        ++lineNbr;
        if (isEmptyOrCommentLine(line, skipSpace) == true)
            continue; // Empty line, or #(comment)
        line.erase(0, skipSpace);
        size_t commentIndex = line.find('#');
        if (commentIndex != string::npos)
            line.erase(commentIndex); // remove comments after text
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
            Logger::logExit(ERROR, "Config error at line", _lines.begin()->first, "invalid block ", _lines.begin()->second);
    }
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
        _validSyntax = false;
        trimLeadingWhitespace(line);
        for (const auto& cmd : cmds)
        {
            if (strncmp(line.c_str(), cmd.first.c_str(), cmd.first.size()) == 0)
            {
                _validSyntax = true;
                cmdCheck(line, block, cmd);
            }
        }
        if (_validSyntax == false)
        {
            for (const auto& whileCmd : whileCmds)
            {
                if (strncmp(line.c_str(), whileCmd.first.c_str(), whileCmd.first.size()) == 0)
                {
                    whileCmdCheck(line, block, whileCmd);
                    _validSyntax = true;
                }
            }
        }
        if (strncmp(line.c_str(), "location", LOCATION_KEYWORD_LENGTH) == 0 && _validSyntax == false)
            LocationCheck(line, block);
    }
    while (checkParseSyntax() == true);
}

/**
 * sets configserver and starts readblock for server
 */
void Parsing::ServerCheck()
{
    ConfigServer curConf;
    curConf.setLineNbr(_lines.begin()->first);
    string line = _lines.begin()->second;
    line.erase(0, SERVER_KEYWORD_LENGTH);
    skipLine(line, false, curConf, true);
    if (line[0] == '{')
    {
        line.erase(0, 1);
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
        Logger::logExit(ERROR, "Config error at line", curConf.getLineNbr(), "Couldn't find opening curly bracket server block");
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
    line.erase(0, cmd.first.size());
    skipLine(line, false, block, false);
    validateWhitespaceAfterCommand(line, WHITESPACE_CHARS, _lines.begin()->first);
    trimLeadingWhitespace(line);
    foundSemicolon = (block.*(cmd.second))(line);
    if (!foundSemicolon)
    {
        skipLine(line, true, block, false);
        if (line[0] != ';')
            Logger::logExit(ERROR, "Config error at line", _lines.begin()->first, "No semi colon found after input");
        line.erase(0, 1);
    }
    skipLine(line, false, block, false);
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
    line.erase(0, cmd.first.size());
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
                Logger::logExit(ERROR, "Config error", '-', "No arguments provided for command '", cmd.first, "' at line ", _lines.begin()->first);
            break;
        }
    }
}

/**
 * adds location block to configServer
 * @param line current line being used for cmd
 * @param block current ConfigServerblock used to add new location block to
 * @param cmd Function to run using line
 */
template <typename T>
void Parsing::LocationCheck(string &line, T &block)
{
    if constexpr (std::is_same<T, ConfigServer>::value) // TODO checks if block == Configserver
    {
        Location location;
        location.setLineNbr(_lines.begin()->first);
        line.erase(0, LOCATION_KEYWORD_LENGTH);
        validateWhitespaceAfterCommand(line, WHITESPACE_CHARS, _lines.begin()->first);
        skipLine(line, false, block, true);
        location.getLocationPath(line);
        skipLine(line, false, block, true);
        if (line[0] != '{')
            Logger::logExit(ERROR, "Config error at line", _lines.begin()->first, "Couldn't find opening curly bracket for location");
        line.erase(0, 1);
        skipLine(line, false, block, false);
        const map<string, bool (Location::*)(string &)> cmds = {
            {"root", &Location::root},
            {"client_max_body_size", &Location::ClientMaxBodysize},
            {"autoindex", &Location::autoIndex},
            {"upload_store", &Location::uploadStore},
            {"cgi_path", &Location::cgiPath}};
        const map<string, bool (Location::*)(string &)> whileCmds = {
            {"error_page", &Location::error_page},
            {"limit_except", &Location::methods},
            {"return", &Location::returnRedirect},
            {"index", &Location::indexPage},
            {"cgi_extension", &Location::cgiExtensions}};
        _lines.begin()->second = line;
        readBlock(location, cmds, whileCmds);
        line = _lines.begin()->second;
        block.addLocation(location, location.getPath());
        _validSyntax = true;
    }
    else
        Logger::logExit(ERROR, "Config error at line", _lines.begin()->first, "Location block can only be used in server block");
}

/**
 * Checks if line contains a closing bracket and handles it
 * @return false if closing bracket found, true otherwise
 */
bool    Parsing::checkParseSyntax()
{
    size_t  skipSpace;
    if (_validSyntax == false)
        Logger::logExit(ERROR, "Config error at line", _lines.begin()->first, "Invalid syntax: ", _lines.begin()->second);

    if (_lines.begin()->second[0] == '}')
    {
        _lines.begin()->second.erase(0, 1);
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
            Logger::logExit(ERROR, "Config error", '-', "Missing bracket, line ", _lines.begin()->first, ": ", _lines.begin()->second);
        _lines.erase(_lines.begin());
        line = _lines.begin()->second;
        curConf.setLineNbr(_lines.begin()->first);
    }
    if (shouldSkipSpace == true)
        trimLeadingWhitespace(line);
}

vector<ConfigServer> &Parsing::getConfigs() {return _configs;}

namespace {
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
            return true;
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
            line.erase(0, skipSpace);
    }

    /**
     * Validates that there is whitespace after a command
     * @param line The line to check
     * @param whitespaceChars The whitespace characters to check for
     * @param lineNumber The line number for error reporting
     * @throws runtime_error if no whitespace found
     */
    void validateWhitespaceAfterCommand(const string &line, const char *whitespaceChars, int lineNumber)
    {
        if (line.empty() || string(whitespaceChars).find(line[0]) == std::string::npos)
            Logger::logExit(ERROR, "Config error at line", lineNumber, "Missing whitespace after command - '", line, "'");
    }
}
