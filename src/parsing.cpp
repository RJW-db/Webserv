#include <Parsing.hpp>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>

bool	skipLine(std::string& line, std::size_t& skipSpace)
{
	skipSpace = line.find_first_not_of(" \t\f\v\r");
	if (std::string::npos == skipSpace || line[skipSpace] == '#')
	{
		return true;
	}
	return false;
}




// std::string	findTerms[10] = {"listen", "location", "root", "server_name", "error_page", "client_max_body_size"};
void Parsing::foundServer(std::fstream& fs, std::string line)
{
	std::size_t skipSpace;
	do
	{
		if (skipLine(line, skipSpace) == true)
			continue;
		if (line[skipSpace] == '{')
			break ;
		else
			throw std::runtime_error("Opening curly bracket not found");
	} while (std::getline(fs, line));

	line = line.substr(skipSpace + 1);

	if (_countServ == 0)
	{
		_confServers = new ConfigServer[_countServ + 1];
	}
	else
	{
		ConfigServer *confServers = new ConfigServer[_countServ + 1];
		for (uint8_t i = 0; i < _countServ; i++)
			confServers[i] = _confServers[i];
		delete[] _confServers;
		_confServers = confServers;
	}

	// std::cout << line << std::endl;
	// exit(0);
    // do
    // {
    //     std::size_t skipSpace;
    //     if (skipLine(line, skipSpace) == true)
    //         continue; // Empty line, or #(comment)
    //     line.find("{");
    //     (void)fs;
    // } while (std::getline(fs, line));
}

Parsing::Parsing(const char *input) : _confServers(NULL), _countServ(0)
{
	std::fstream fs;
	fs.open(input, std::fstream::in);

	if (fs.is_open() == false)
		throw std::runtime_error("klopt");
	std::string line;
	std::string all_lines;
	while (std::getline(fs, line))
	{
		std::size_t skipSpace;
		if (skipLine(line, skipSpace) == true)
			continue; // Empty line, or #(comment)
		if (line.find("server", skipSpace) == skipSpace)
		{
			std::cout << "bur" << std::endl;
			foundServer(fs, line.substr(skipSpace + 6));
		}
		else
		{
			throw std::runtime_error("Invalid found in .conf");
		}
	}
	fs.close();


}

Parsing::~Parsing()
{
	
}
