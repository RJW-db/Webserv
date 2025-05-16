#ifndef CONFIGSERVER_HPP
#define CONFIGSERVER_HPP

#include <map>
#include <string>
#include <stdint.h>	// uint16_t, cstdint doesn't exist in std=98
#include <fstream>


class ConfigServer
{
	public:
		// ConfigServer(std::fstream &fs);
		ConfigServer(const ConfigServer& other);
		~ConfigServer();


		ConfigServer& operator=(const ConfigServer& other);

		std::string listenHostname(std::string line);

	private: 
		std::multimap<std::string, uint16_t> _hostAddress;
};

#endif