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


// void Parsing::readLocation(ConfigServer &curConf)
// {
// 	Location locationBlock;
// 	string (Location::*funcs[2])(string, bool &) = {&Location::root, &Location::ClientMaxBodysize};
// 	const std::array<std::string, 2> cmds_strings = {"root", "client_max_body_size"};

// 	string (Location::*whileFuncs[1])(string, bool &) = {&Location::error_page};
// 	const std::array<std::string, 2> cmdsStringsWhile = {"error_page"};
// 	if (strncmp(_lines[0].c_str(), "location", 8) == 0)
// 	{
// 		bool findColon;
// 		size_t skipSpace;
// 		_lines[0] = _lines[0].substr(8);
// 		if (skipLine(_lines[0], skipSpace) == true)
// 			_lines.erase(_lines.begin());
// 		_lines[0] = ftSkipspace(_lines[0]);
// 		_lines[0] = locationBlock.setPath(_lines[0]);
// 		_lines[0] = ftSkipspace(_lines[0]);
// 		while (1)
// 		{
// 			_lines[0] = ftSkipspace(_lines[0]);
// 			for (size_t i = 0; i < cmds_strings.max_size(); i++)
// 			{
// 				if (strncmp(_lines[0].c_str(), cmds_strings[i].c_str(), cmds_strings[i].size()) == 0)
// 				{
// 					_lines[0] = _lines[0].substr(cmds_strings[i].size());
// 					if (skipLine(_lines[0], skipSpace) == true)
// 						_lines.erase(_lines.begin());
// 					else if (string(" \t\f\v\r").find(_lines[0][0]) == std::string::npos)
// 						throw runtime_error("no space found after command");
// 					_lines[0] = ftSkipspace(_lines[0]);
// 					_lines[0] = (locationBlock.*(funcs[i]))(_lines[0], findColon);
// 					if (findColon == false)
// 					{
// 						_lines.erase(_lines.begin());
// 						if (_lines[0][_lines[0].find_first_not_of(" \t\f\v\r")] != ';')
// 							throw runtime_error("no semi colon found after cmd");
// 					}
// 				}
// 			}
// 			for (size_t i = 0; i < cmdsStringsWhile.max_size(); i++)
// 			{
// 				if (strncmp(_lines[0].c_str(), cmdsStringsWhile[i].c_str(), cmdsStringsWhile[i].size()) == 0)
// 				{
// 					_lines[0] = _lines[0].substr(cmdsStringsWhile[i].size());
// 					if (string(" \t\f\v\r\n").find(_lines[0][0]) == std::string::npos)
// 						throw runtime_error("no space found after command");
// 					while (1)
// 					{
// 						bool findColon = false;
// 						if (skipLine(_lines[0], skipSpace) == true)
// 							_lines.erase(_lines.begin());
// 						_lines[0] = ftSkipspace(_lines[0]);
// 						_lines[0] = (locationBlock.*(whileFuncs[i]))(_lines[0], findColon);
// 						if (skipLine(_lines[0], skipSpace) == true)
// 							_lines.erase(_lines.begin());
// 						if (findColon == true)
// 						{
// 							_lines[0] = ftSkipspace(_lines[0]);
// 							if (_lines[0][0] != ';')
// 								throw runtime_error("no semi colon found after error_page");
// 							_lines[0] = _lines[0].substr(1);
// 							_lines[0] = ftSkipspace(_lines[0]);
// 							if (skipLine(_lines[0], skipSpace) == true)
// 								_lines.erase(_lines.begin());
// 							break ;
// 						}
// 					}
// 				}
// 			}
// 		}
// 	}
// }

// string	findTerms[10] = {"listen", "location", "root", "server_name", "error_page", "client_max_body_size"};
// void Parsing::readServer()
// {
// 	string (ConfigServer::*funcs[3])(string, bool &) = {&ConfigServer::listenHostname, &ConfigServer::root, &ConfigServer::ClientMaxBodysize};
// 	const std::array<std::string, 3> cmds_strings = {"listen", "root", "client_max_body_size"};
// 	ConfigServer curConf;
// 	int i = 0;
// 	while (1) // need to add check for end and invalid command
// 	{
// 		bool findColon;
// 		size_t skipSpace;
// 		_lines[0] = ftSkipspace(_lines[0]);
// 		for (size_t i = 0; i < cmds_strings.max_size(); i++)
// 		{
// 			if (strncmp(_lines[0].c_str(), cmds_strings[i].c_str(), cmds_strings[i].size()) == 0)
// 			{
// 				_lines[0] = _lines[0].substr(cmds_strings[i].size());
// 				if (skipLine(_lines[0], skipSpace) == true)
// 					_lines.erase(_lines.begin());
// 				else if (string(" \t\f\v\r").find(_lines[0][0]) == std::string::npos)
// 					throw runtime_error("no space found after command");
// 				_lines[0] = ftSkipspace(_lines[0]);
// 				_lines[0] = (curConf.*(funcs[i]))(_lines[0], findColon);
// 				if (findColon == false)
// 				{
// 					_lines.erase(_lines.begin());
// 					if (_lines[0][_lines[0].find_first_not_of(" \t\f\v\r")] != ';')
// 						throw runtime_error("no semi colon found after cmd");
// 				}
// 			}
// 		}
// 		if (strncmp(_lines[0].c_str(), "error_page", 10) == 0)
// 		{
// 			_lines[0] = _lines[0].substr(10);
// 			if (string(" \t\f\v\r\n").find(_lines[0][0]) == std::string::npos)
// 				throw runtime_error("no space found after command");
// 			while (1)
// 			{
// 				findColon = false;
// 				if (skipLine(_lines[0], skipSpace) == true)
// 					_lines.erase(_lines.begin());
// 				_lines[0] = ftSkipspace(_lines[0]);
// 				_lines[0] = curConf.error_page(_lines[0], findColon);
// 				if (skipLine(_lines[0], skipSpace) == true)
// 					_lines.erase(_lines.begin());
// 				if (findColon == true)
// 				{
// 					_lines[0] = ftSkipspace(_lines[0]);
// 					if (_lines[0][0] != ';')
// 						throw runtime_error("no semi colon found after error_page");
// 					_lines[0] = _lines[0].substr(1);
// 					_lines[0] = ftSkipspace(_lines[0]);
// 					if (skipLine(_lines[0], skipSpace) == true)
// 						_lines.erase(_lines.begin());
// 					break ;
// 				}
// 			}
// 		}
// 		if (i == 100)
// 			break ;
// 		++i;
// 	}
	
// 	// _configs.insert(_configs.end(), curConf);
// 	_configs.insert(_configs.end(), curConf);
// }

// void Parsing::readBlock(T &block, 
//                         const std::array<std::string, 3> &cmds_strings, 
//                         string (T::*funcs[])(string, bool &), 
//                         const std::array<std::string, 1> &whileCmdsStrings = {}, 
//                         string (T::*whileFuncs[])(string, bool &) = nullptr)
template <typename T>
void Parsing::readBlock(T &block, 
	const map<string, string (T::*)(string, bool &)> &cmds, 
	const map<string, string (T::*)(string, bool &)> &whileCmds)
{
    size_t skipSpace;
    bool findColon;
	size_t j = 0;
	cout << "hier" << endl;
	while (1)
    {
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
                }
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
                        // _lines[0] = ftSkipspace(_lines[0]);
                        // if (_lines[0][0] != ';')
                        //     throw runtime_error("no semi colon found after error_page");
                        // _lines[0] = _lines[0].substr(1);
                        // _lines[0] = ftSkipspace(_lines[0]);
                        // if (skipLine(_lines[0], skipSpace))
                        //     _lines.erase(_lines.begin());
                        // break;
                    }
                }
            }
        }
		// if constexpr (std::is_same<T, ConfigServer>::value && strncmp(_lines[0].c_str(), "location", 8) == 0) //TODO check if in server lock
		if constexpr (std::is_same<T, ConfigServer>::value)
		{
			if (strncmp(_lines[0].c_str(), "location", 8) == 0)
			{
				Location location;
				const std::map<std::string, string (Location::*)(string, bool &)> cmds = {
					{"root", &Location::root},
					{"client_max_body_size", &Location::ClientMaxBodysize}
				};
				const std::map<std::string, string (Location::*)(string, bool &)> whileCmds = {
					{"error_page", &Location::error_page}
				};
				readBlock(location, cmds, whileCmds);
				block.locations.push_back(location);
			}
		}
		if (++j == 100)
			break ;
        if (_lines.empty())
            break;
    }
}

// ClientMaxBodysize()
// client_max_body_size 10M;
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
				{"client_max_body_size", &ConfigServer::ClientMaxBodysize}};
			const std::map<std::string, string (ConfigServer::*)(string, bool &)> whileCmds = {
					{"error_page", &ConfigServer::error_page}};
			readBlock(curConf, cmds, whileCmds);
			_configs.push_back(curConf);
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
