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

template <typename T>
void Parsing::skipLine(string &line, bool forceSkip, T &curConf)
{
	size_t skipSpace = line.find_first_not_of(" \t\f\v\r");
    if (string::npos == skipSpace || forceSkip)
    {
		if (_lines.size() <= 1)
			throw runtime_error("No closing bracket found after block: " + to_string(_lines.begin()->first));
		_lines.erase(_lines.begin());
        line = _lines.begin()->second;
		curConf.setLineNbr(_lines.begin()->first);
    }
}

template <typename T>
void Parsing::LocationCheck(string &line, T &block, bool &validSyntax)
{
    if constexpr (std::is_same<T, ConfigServer>::value) // checks i block == Configserver
    {
        line = line.substr(8);
        if (string(" \t\f\v\r").find(line[0]) == std::string::npos)
            throw runtime_error("no space found after command");
        Location location;
        location.setLineNbr(_lines.begin()->first);
        skipLine(line, false, block);
        ftSkipspace(line);
        string path = location.getLocationPath(line);
        skipLine(line, false, block);
        ftSkipspace(line);
        if (line[0] != '{')
            throw runtime_error("couldn't find opening curly bracket for location");
        line = line.substr(1);
        skipLine(line, false, block);
        ftSkipspace(line);
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
        readBlock(location, cmds, whileCmds);
        line = _lines.begin()->second;
        block.addLocation(location, path);
        validSyntax = true;
    }
    else
        throw runtime_error(to_string(_lines.begin()->first) + ": location block can only be used in server block");
}

template <typename T>
void Parsing::whileCmdCheck(string &line, T &block, const pair<string, bool (T::*)(string &)> &cmd)
{
	bool findColon;
	line = line.substr(cmd.first.size());
	if (string(" \t\f\v\r\n").find(line[0]) == std::string::npos)
		throw runtime_error("no space found after command");
	size_t run = 0;
	while (++run)
	{
		skipLine(line, false, block);
		ftSkipspace(line);
		findColon = (block.*(cmd.second))(line);
		skipLine(line, false, block);
		if (findColon == true)
		{
			if (run == 1)
				throw runtime_error(string("no arguments after") + cmd.first);
			break;
		}
	}
}

template <typename T>
void Parsing::cmdCheck(string &line, T &block, const pair<string, bool (T::*)(string &)> &cmd)
{
	bool findColon;
    line = line.substr(cmd.first.size());
    skipLine(line, false, block);
    if (string(" \t\f\v\r").find(line[0]) == std::string::npos)
        throw runtime_error("no space found after command");
    ftSkipspace(line);
    findColon = (block.*(cmd.second))(line);
    if (!findColon)
    {
        skipLine(line, true, block);
        if (line[0] != ';')
            throw runtime_error(to_string(_lines.begin()->first) + ": no semi colon found after input");
        line = line.substr(1);
    }
    skipLine(line, false, block);
}

template <typename T>
void Parsing::readBlock(T &block, 
    const map<string, bool (T::*)(string &)> &cmds, 
    const map<string, bool (T::*)(string &)> &whileCmds)
{
    string line = _lines.begin()->second;
    do
    {
        validSyntax = false;
        ftSkipspace(line);
        for (const pair<string, bool (T::*)(string &)> &cmd : cmds)
        {
            if (strncmp(line.c_str(), cmd.first.c_str(), cmd.first.size()) == 0)
			{
    			validSyntax = true;
                cmdCheck(line, block, cmd);
			}
        }
        for (const pair<string, bool (T::*)(string &)> &whileCmd : whileCmds)
        {
            if (strncmp(line.c_str(), whileCmd.first.c_str(), whileCmd.first.size()) == 0)
            {
                whileCmdCheck(line, block, whileCmd);
                validSyntax = true;
            }
        }
        if (strncmp(_lines.begin()->second.c_str(), "location", 8) == 0)
            LocationCheck(line, block, validSyntax);
    } 
	while (runReadblock() == true);
}

void Parsing::ServerCheck()
{
    ConfigServer curConf;
    curConf.setLineNbr(_lines.begin()->first);
    _lines.begin()->second = _lines.begin()->second.substr(6);
    skipLine(_lines.begin()->second, false, curConf);
    if (_lines.begin()->second[0] == '{')
    {
        _lines.begin()->second = _lines.begin()->second.substr(1);
        skipLine(_lines.begin()->second, false, curConf);
        const std::map<string, bool (ConfigServer::*)(string &)> cmds = {
            {"listen", &ConfigServer::listenHostname},
            {"root", &ConfigServer::root},
            {"client_max_body_size", &ConfigServer::ClientMaxBodysize},
            {"server_name", &ConfigServer::serverName},
            {"autoindex", &Location::autoIndex}};
        const map<string, bool (ConfigServer::*)(string &)> whileCmds = {
            {"error_page", &ConfigServer::error_page},
            {"return", &ConfigServer::returnRedirect}};
        readBlock(curConf, cmds, whileCmds);
        curConf.setDefaultConf();
        _configs.push_back(curConf);
    }
    else
        throw runtime_error("Couldn't find closing curly bracket server block");
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
        if (_lines.empty())
            break ; 
        if (strncmp(_lines.begin()->second.c_str(), "server", 6) == 0)
            ServerCheck();
        else
            throw runtime_error("Invalid line found expecting server" + _lines[0]);
    }
}

Parsing::~Parsing()
{
    
}

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
			cout << "  Client Max Body Size: " << config.getClientBodySize() << endl;
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
			Location &location = pair.second;
			cout << "location: " << "Path: " << pair.first << endl;
			cout << "  Root: " << location.getRoot() << endl;
			cout << "  Client Max Body Size: " << location.getClientBodySize() << endl;
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
			for (const string &method : location.getMethods())
			{
				cout << method << " ";
			}
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