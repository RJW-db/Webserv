#ifndef PARSING_HPP
# define PARSING_HPP

// #include <cstring>
#include <string>
#include <ConfigServer.hpp>
#include <stdint.h>	// uint16_t, cstdint doesn't exist in std=98

class Parsing
{
	public:
		Parsing(const char *input);
		~Parsing();
	private:
		void foundServer(std::fstream& fs, std::string line);


		ConfigServer	*_confServers;
		uint8_t 		_countServ;
};

#endif
