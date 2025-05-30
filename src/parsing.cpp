#include <Parsing.hpp>
#include <ConfigServer.hpp>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <Location.hpp>

using namespace std;

bool    emptyLine(string &line, size_t &skipSpace)
{
    skipSpace = line.find_first_not_of(" \t\f\v\r");
    if (string::npos == skipSpace || line[skipSpace] == '#')
    {
        return true;
    }
    return false;
}

// string ftSkipspace(string &line)
// {
//     size_t skipSpace = line.find_first_not_of(" \t\f\v\r");
//     if (skipSpace != 0 && skipSpace != string::npos)
//         return line.substr(skipSpace);
//     return line;
// }


void ftSkipspace(string &line)
{
    size_t skipSpace = line.find_first_not_of(" \t\f\v\r");
    if (skipSpace != 0 && skipSpace != string::npos)
        line = line.substr(skipSpace);
    // return line;
}

bool    Parsing::runReadblock()
{
    size_t  skipSpace;
    if (_lines.begin()->second[0] == '}')
    {
        _lines.begin()->second =  _lines.begin()->second.substr(1);
        if (emptyLine(_lines.begin()->second, skipSpace) == true)
            _lines.erase(_lines.begin());
        return false;
    }
    if (validSyntax == false)
        throw runtime_error("invalid syntax: " + _lines.begin()->second);
    return true;
}

string Parsing::skipLine(string &line, bool forceSkip)
{
	size_t skipSpace = line.find_first_not_of(" \t\f\v\r");
    if (string::npos == skipSpace)
    {
		_lines.erase(_lines.begin());
        return _lines.begin()->second;
    }
    return line;
}
//still need to add check for no lines left
template <typename T>
void Parsing::readBlock(T &block, 
    const map<string, string (T::*)(string, bool &)> &cmds, 
    const map<string, string (T::*)(string, bool &)> &whileCmds)
{
    size_t skipSpace;
    bool findColon;
    string line = _lines.begin()->second;
    do
    {
		cout << "line: " << line <<": " << _lines.begin()->first << endl;
        validSyntax = false;
        ftSkipspace(line);
        for (const auto &cmd : cmds)
        {
            if (strncmp(line.c_str(), cmd.first.c_str(), cmd.first.size()) == 0)
            {
                line = line.substr(cmd.first.size());
                line = skipLine(line, false);
                if (string(" \t\f\v\r").find(line[0]) == std::string::npos)
                    throw runtime_error("no space found after command");
                ftSkipspace(line);
                line = (block.*(cmd.second))(line, findColon);
                if (!findColon)
                {
                	line = skipLine(line, true);
                    if (line[0] != ';')
                        throw runtime_error("no semi colon found after cmd");
                    line = line.substr(1);
                }
                line = skipLine(line, false);
                validSyntax = true;
            }
        }
        for (const auto &whileCmd : whileCmds)
        {
            if (strncmp(line.c_str(), whileCmd.first.c_str(), whileCmd.first.size()) == 0)
            {
                line = line.substr(whileCmd.first.size());
                if (string(" \t\f\v\r\n").find(line[0]) == std::string::npos)
                    throw runtime_error("no space found after command");
                size_t run = 0;
                while (++run)
                {
                    findColon = false;
                    line = skipLine(line, false);
                    ftSkipspace(line);
                    line = (block.*(whileCmd.second))(line, findColon);
                    line = skipLine(line, false);
                    if (findColon == true)
                    {
                        if (run == 1)
                            throw runtime_error(string("no arguments after") + whileCmd.first);
                        break ;
                    }
                }
                validSyntax = true;
            }
        }
        if constexpr (std::is_same<T, ConfigServer>::value)
        {
            if (strncmp(_lines.begin()->second.c_str(), "location", 8) == 0)
            {
                line = line.substr(8);
				if (string(" \t\f\v\r").find(line[0]) == std::string::npos)
                    throw runtime_error("no space found after command");
                Location location;
                line = skipLine(line, false);
                ftSkipspace(line);
                line = location.setPath(line);
                line = skipLine(line, false);
                ftSkipspace(line);
                if (line[0] != '{')
                    throw runtime_error("couldn't find opening curly bracket for location");
                line =line.substr(1);
                line = skipLine(line, false);
                ftSkipspace(line);
                const std::map<std::string, string (Location::*)(string, bool &)> cmds = {
                    {"root", &Location::root},
                    {"client_max_body_size", &Location::ClientMaxBodysize},
                    {"autoindex", &Location::autoIndex},
                    {"return", &Location::returnRedirect}, 
                    {"upload_store", &Location::uploadStore}};
                const std::map<std::string, string (Location::*)(string, bool &)> whileCmds = {
                    {"error_page", &Location::error_page}, 
                    {"limit_except", &Location::methods}, 
                    {"index", &Location::indexPage}};
                readBlock(location, cmds, whileCmds);
				line = _lines.begin()->second;
                block._locations.push_back(location);
                validSyntax = true;
            }
        }
    } while (runReadblock() == true);
}

static void setErrorPages(map<uint16_t, string> &ErrorCodesWithPage, Aconfig &curConf)
{
    uint16_t errorCodes[11] = {400, 403, 404, 405, 413, 431, 500, 501, 502, 503, 504};

    for (uint16_t errorCode : errorCodes)
    {
        if (ErrorCodesWithPage.find(errorCode) == ErrorCodesWithPage.end())
        {
            string fileName = "webPages/defaultErrorPages/" + to_string(static_cast<int>(errorCode)) + ".html";
            ErrorCodesWithPage.insert({errorCode, fileName});
            std::cout << "error page: " << ErrorCodesWithPage.at(errorCode) << " code:" << errorCode << std::endl;
        }
        else
        {
            ErrorCodesWithPage.at(errorCode).insert(0, curConf._root);
            ErrorCodesWithPage.at(errorCode) = ErrorCodesWithPage.at(errorCode).substr(1);
            if (ErrorCodesWithPage.at(errorCode)[0] == '/')
                ErrorCodesWithPage.at(errorCode) = ErrorCodesWithPage.at(errorCode).substr(1);
            std::cout << "error page: " << ErrorCodesWithPage.at(errorCode) << " code:" << errorCode << std::endl;
        }
        if (access(ErrorCodesWithPage.at(errorCode).c_str(), R_OK) != 0)
            throw runtime_error("couldn't open error page:" + ErrorCodesWithPage.at(errorCode));
    }
}

static void setLocation(Location &curLocation, ConfigServer &curConf)
{
    if (curLocation._autoIndex == autoIndexNotFound)
        curLocation._autoIndex = curConf._autoIndex;
    if (curLocation._root.empty())
        curLocation._root = curConf._root;
    if (curLocation._clientBodySize == 0)
        curLocation._clientBodySize = curConf._clientBodySize;
    if (curLocation._returnRedirect.first == 0)
        curLocation._returnRedirect = curConf._returnRedirect;
    for (pair<uint16_t, string> errorCodePages : curConf.ErrorCodesWithPage)
        curLocation.ErrorCodesWithPage.insert(errorCodePages);
    if (curLocation._indexPage.empty())
    {
        for (string indexPage : curConf._indexPage)
            curLocation._indexPage.push_back(indexPage);
    }
    if (curLocation._upload_store.empty())
        curLocation._upload_store = curLocation._root;
    if (curLocation._methods[0].empty())
    {
        curLocation._methods[0] = "GET";
        curLocation._methods[1] = "POST";
        curLocation._methods[2] = "DELETE";
    }
    setErrorPages(curLocation.ErrorCodesWithPage, curLocation);
}

static void setConf(ConfigServer &curConf)
{
    //what to do with no error pages? do we create our own or just have one
    if (curConf._root.empty())
        curConf._root = "/var/www"; // what default root should we use?
    if (curConf._clientBodySize == 0)
        curConf._clientBodySize = 1024 * 1024;
    if (curConf._autoIndex == autoIndexNotFound)
        curConf._autoIndex = autoIndexFalse;
    if (curConf._hostAddress.empty())
    {
        bool tmp;
        curConf.listenHostname("80;", tmp); //sets sockaddr ip to 0.0.0.0 and port to 80
    }

    for(Location &location : curConf._locations)
        setLocation(location, curConf);
    setErrorPages(curConf.ErrorCodesWithPage, curConf);
}

Parsing::Parsing(const char *input) /* :  _confServers(NULL), _countServ(0)  */
{
    fstream fs;
    fs.open(input, fstream::in);

    if (fs.is_open() == false)
        throw runtime_error("inputfile couldn't be");
    string line;
    size_t skipSpace;
    size_t lineNbr = 0;
    while (getline(fs, line))
    {
        ++lineNbr;
        if (emptyLine(line, skipSpace) == true)
            continue; // Empty line, or #(comment)
        line = line.substr(skipSpace);
        size_t commentindex = line.find('#');
        if (commentindex != string::npos)
            line = line.substr(0, commentindex); // remove comments after text
        _lines.insert({lineNbr, line});
    }
    fs.close();
    while (1)
    {
        if (strncmp(_lines.begin()->second.c_str(), "server", 6) == 0)
        {
            _lines.begin()->second = _lines.begin()->second.substr(6);
            if (emptyLine(_lines.begin()->second, skipSpace) == true)
                _lines.erase(_lines.begin());
            if (_lines.begin()->second[0] == '{')
            {
                _lines.begin()->second = _lines.begin()->second.substr(1);
                if (emptyLine(line, skipSpace))
                {
					line = _lines.begin()->second;
                    _lines.erase(_lines.begin());
                }
                ConfigServer curConf;
                const std::map<std::string, string (ConfigServer::*)(string, bool &)> cmds = {
                    {"listen", &ConfigServer::listenHostname},
                    {"root", &ConfigServer::root},
                    {"client_max_body_size", &ConfigServer::ClientMaxBodysize},
                    {"server_name", &ConfigServer::serverName},
                    {"autoindex", &Location::autoIndex}};
                const std::map<std::string, string (ConfigServer::*)(string, bool &)> whileCmds = {
                    {"error_page", &ConfigServer::error_page},
                    {"return", &ConfigServer::returnRedirect}};
                readBlock(curConf, cmds, whileCmds);
                setConf(curConf);
                _configs.push_back(curConf);
            }
            else
                throw runtime_error("Invalid line found expecting server" + _lines[0]);
        }
        else if (_lines.size() == 0)
            break ; 
        else
            throw runtime_error("Couldn't find closing curly bracket server block");
    }
}

Parsing::~Parsing()
{
    
}
