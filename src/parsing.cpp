#include <Parsing.hpp>
#include <ConfigServer.hpp>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <Location.hpp>

using namespace std;

bool	skipLine(string &line, size_t &skipSpace)
{
    skipSpace = line.find_first_not_of(" \t\f\v\r");
    if (string::npos == skipSpace || line[skipSpace] == '#')
    {
        return true;
    }
    return false;
}

string ftSkipspace(string &line)
{
    size_t skipSpace = line.find_first_not_of(" \t\f\v\r");
    if (skipSpace != 0 && skipSpace != string::npos)
        return line.substr(skipSpace);
    return line;
}

bool    Parsing::runReadblock()
{
    size_t  skipSpace;
    if (_lines[0][0] == '}')
    {
        _lines[0] =  _lines[0].substr(1);
        if (skipLine(_lines[0], skipSpace) == true)
            _lines.erase(_lines.begin());
        return false;
    }
    if (validSyntax == false)
        throw runtime_error("invalid syntax: " + _lines[0]);
    return true;
}

template <typename T>
void Parsing::readBlock(T &block, 
    const map<string, string (T::*)(string, bool &)> &cmds, 
    const map<string, string (T::*)(string, bool &)> &whileCmds)
{
    size_t skipSpace;
    bool findColon;
    do
    {
        validSyntax = false;
        _lines[0] = ftSkipspace(_lines[0]);
        for (const auto &cmd : cmds)
        {
            if (strncmp(_lines[0].c_str(), cmd.first.c_str(), cmd.first.size()) == 0)
            {
                _lines[0] = _lines[0].substr(cmd.first.size());
                if (skipLine(_lines[0], skipSpace))
                    _lines.erase(_lines.begin());
                else if (string(" \t\f\v\r").find(_lines[0][0]) == std::string::npos)
                    throw runtime_error("no space found after command");
                _lines[0] = ftSkipspace(_lines[0]);
                _lines[0] = (block.*(cmd.second))(_lines[0], findColon);
                if (!findColon)
                {
                    _lines.erase(_lines.begin());
                    if (_lines[0][_lines[0].find_first_not_of(" \t\f\v\r")] != ';')
                        throw runtime_error("no semi colon found after cmd");
                    _lines[0] = _lines[0].substr(1);
                }
                if (skipLine(_lines[0], skipSpace))
                    _lines.erase(_lines.begin());
                validSyntax = true;
            }
        }
        for (const auto &whileCmd : whileCmds)
        {
            if (strncmp(_lines[0].c_str(), whileCmd.first.c_str(), whileCmd.first.size()) == 0)
            {
                _lines[0] = _lines[0].substr(whileCmd.first.size());
                if (string(" \t\f\v\r\n").find(_lines[0][0]) == std::string::npos)
                    throw runtime_error("no space found after command");
                size_t run = 0;
                while (++run)
                {
                    findColon = false;
                    if (skipLine(_lines[0], skipSpace))
                        _lines.erase(_lines.begin());
                    _lines[0] = ftSkipspace(_lines[0]);
                    _lines[0] = (block.*(whileCmd.second))(_lines[0], findColon);
                    if (skipLine(_lines[0], skipSpace))
                        _lines.erase(_lines.begin());
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
        // std::cout << validSyntax << std::endl;
        // std::cout << _lines[0] << std::endl;
        if constexpr (std::is_same<T, ConfigServer>::value)
        {
            if (strncmp(_lines[0].c_str(), "location", 8) == 0)
            {
                _lines[0] = _lines[0].substr(8);
                Location location;
                if (skipLine(_lines[0], skipSpace) == true)
                    _lines.erase(_lines.begin());
                _lines[0] = ftSkipspace(_lines[0]);
                _lines[0] = location.setPath(_lines[0]);
                if (skipLine(_lines[0], skipSpace) == true)
                    _lines.erase(_lines.begin());
                _lines[0] = ftSkipspace(_lines[0]);
                if (_lines[0][0] != '{')
                    throw runtime_error("couldn't find opening curly bracket for location");
                _lines[0] = _lines[0].substr(1);
                if (skipLine(_lines[0], skipSpace) == true)
                    _lines.erase(_lines.begin());
                _lines[0] = ftSkipspace(_lines[0]);
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
                block._locations.push_back(location);
                validSyntax = true;
            }
        }
    } while (runReadblock() == true);
}

static void setLocation(Location &curLocation, ConfigServer &curConf)
{
    // what to do for methods (only get if doesn't exist?)
    // how to check if autoindex turned off for location block?
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
    if (curLocation._upload_store.empty())     //what to do for upload store if post in limit_except
        curLocation._upload_store = curLocation._root;
    
}

static void setConf(ConfigServer &curConf)
{
    // we don't need to have returnredirect or indexPage
    //what to do with no error pages? do we create our own or just have one
    if (curConf._root.empty())
        curConf._root = "/var/www"; // what default root should we use?
    if (curConf._clientBodySize == 0)
        curConf._clientBodySize = 1024 * 1024;
    if (curConf._hostAddress.empty())
    {
        bool tmp;
        curConf.listenHostname("80;", tmp);
    }
}

Parsing::Parsing(const char *input) /* :  _confServers(NULL), _countServ(0)  */
{
    fstream fs;
    fs.open(input, fstream::in);

    if (fs.is_open() == false)
        throw runtime_error("inputfile couldn't be");
    string line;
    size_t skipSpace;
    while (getline(fs, line))
    {
        if (skipLine(line, skipSpace) == true)
            continue; // Empty line, or #(comment)
        line = line.substr(skipSpace);
        size_t commentindex = line.find('#');
        if (commentindex != string::npos)
            line = line.substr(0, commentindex); // remove comments after text
        
        _lines.push_back(line);
    }
    fs.close();
    if (strncmp(_lines[0].c_str(), "server", 6) == 0)
    {
        _lines[0] = _lines[0].substr(6);
        if (skipLine(_lines[0], skipSpace) == true)
            _lines.erase(_lines.begin());
        if (_lines[0][0] == '{')
        {
            _lines[0] = _lines[0].substr(1);
            if (skipLine(_lines[0], skipSpace) == true)
                _lines.erase(_lines.begin());
            // cout << "found server" << endl;
            ConfigServer curConf;
            const std::map<std::string, string (ConfigServer::*)(string, bool &)> cmds = {
                {"listen", &ConfigServer::listenHostname},
                {"root", &ConfigServer::root},
                {"client_max_body_size", &ConfigServer::ClientMaxBodysize},
                {"server_name", &ConfigServer::serverName},
                {"autoindex", &Location::autoIndex}};
            const std::map<std::string, string (ConfigServer::*)(string, bool &)> whileCmds = {
                    {"error_page", &ConfigServer::error_page},
                    {"return", &Location::returnRedirect}};
            readBlock(curConf, cmds, whileCmds);
            _configs.push_back(curConf);
            // std::cout << "Number of location blocks _configs: " << _configs[0].locations.size() << std::endl;

            // std::cout << "Number of location blocks curConf: " << curConf.locations.size() << std::endl;
            
            // cout << curConf.locations.size() << endl;
        }
    }
    else
        throw runtime_error("help");
    // for (string line: _lines)
    // {
    // 	cout << line << endl;
    // }

}

Parsing::~Parsing()
{
    
}
