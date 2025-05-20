#include <Parsing.hpp>
#include <ConfigServer.hpp>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>

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




// void Parsing::foundServer(fstream &fs, string line)
// {
	// size_t skipSpace;
	// do
	// {
	// 	if (skipLine(line, skipSpace) == true)
	// 		continue;
	// 	if (line[skipSpace] == '{')
	// 		break ;
	// 	else
	// 		throw runtime_error("Opening curly bracket not found");
	// } while (getline(fs, line));

	// line = line.substr(skipSpace + 1);

	// if (_countServ == 0)
	// {
	// 	_confServers = new ConfigServer[_countServ + 1] { ConfigServer(fs) };
	// }
	// else
	// {
	// 	ConfigServer *confServers = new ConfigServer[_countServ + 1];
	// 	for (uint8_t i = 0; i < _countServ; i++)
	// 		confServers[i] = _confServers[i];
	// 	delete[] _confServers;
	// 	_confServers = confServers;
	// }

	// cout << line << endl;
	// exit(0);
    // do
    // {
    //     size_t skipSpace;
    //     if (skipLine(line, skipSpace) == true)
    //         continue; // Empty line, or #(comment)
    //     line.find("{");
    //     (void)fs;
    // } while (getline(fs, line));
// }




// sockaddr_in Parsing::listenHostname()
// { //to do should we store in addrinfo immediately? and how to handle fstream
// 	sockaddr_in sockadr;
// 	std::size_t skipHostname = _lines[0].find_first_not_of("0123456789.");
// 	if (_lines[0][skipHostname] == ';')
// 	{
// 		if (_lines[0].find('.') < skipHostname)
// 			throw std::runtime_error("listen port contains .");
// 		sockadr.sin_port = stoi(_lines[0]);
// 		if (sockadr.sin_port <= 0)
// 			throw runtime_error("error found in port handling conf");
// 		// sockadr.sin_addr.s_addr = htonl();
	
// 		return (line.substr(skipSpace + skipHostname + 1));
// 	}
// 	else if (line[skipHostname + skipSpace] == ':')
// 	{
// 		std::string hostname = line.substr(skipSpace, skipHostname - skipSpace);
// 		_hostAddress.insert({hostname, std::stoi(line.substr(skipSpace + skipHostname + 1))}); //to do check for missing port
// 		std::size_t pos = line.find_first_not_of("0123456789", skipSpace + skipHostname + 1);
// 		if (pos != std::string::npos & &line[skipSpace + skipHostname + pos] == ';')
// 			throw std::runtime_error("invalid character found after listen hostname and port");
// 		return (line.substr(skipSpace + skipHostname + pos));
// 	}
// 	else
// 		throw std::runtime_error("invalid character found after listen");
// }


// string	findTerms[10] = {"listen", "location", "root", "server_name", "error_page", "client_max_body_size"};
void Parsing::readServer()
{
	string (ConfigServer::*funcs[2])(string, bool &) = {&ConfigServer::listenHostname, &ConfigServer::root};
	const string cmds_strings[2] = {"listen", "root"};
	ConfigServer curConf;
	while (1)
	{
		bool findColon;
		size_t skipSpace = _lines[0].find_first_not_of(" \t\f\v\r");
		_lines[0] = _lines[0].substr(skipSpace);
		for (size_t i = 0; i < 1; i++)
		{
			if (_lines[0].find(cmds_strings[i].c_str(), 0, cmds_strings[i].size()) != string::npos)
			{
				_lines[0] = _lines[0].substr(cmds_strings[i].size());
				if (skipLine(_lines[0], skipSpace) == true)
					_lines.erase(_lines.begin());
				skipSpace = _lines[0].find_first_not_of(" \t\f\v\r");
				_lines[0] = _lines[0].substr(skipSpace);
				_lines[0] = (curConf.*(funcs[i]))(_lines[0], findColon);
				if (findColon == false)
				{
					_lines.erase(_lines.begin());
					if (_lines[0][_lines[0].find_first_not_of(" \t\f\v\r")] != ';')
						throw runtime_error("no semi colon found after cmd");
				}
			}
			// if (_lines[0].find("location", 0, 8))
			// {

			// }
		}
		break ;
	}
	_configs.insert(_configs.end(), curConf);
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
	if (_lines[0].find("server", 0, 6) != string::npos)
	{
		_lines[0] = _lines[0].substr(6);
		if (skipLine(_lines[0], skipSpace) == true)
			_lines.erase(_lines.begin());
		if (_lines[0][0] == '{')
		{
			_lines[0] = _lines[0].substr(1);
			if (skipLine(_lines[0], skipSpace) == true)
				_lines.erase(_lines.begin());
			cout << "found server" << endl;
			readServer();
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
