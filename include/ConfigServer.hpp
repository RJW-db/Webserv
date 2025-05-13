#ifndef CONFIGSERVER_HPP
#define CONFIGSERVER_HPP

#include <map>
#include <string>
#include <stdint.h>	// uint16_t, cstdint doesn't exist in std=98

class ConfigServer
{
	public:
		ConfigServer();
		ConfigServer(const ConfigServer& other);
		~ConfigServer();


		ConfigServer& operator=(const ConfigServer& other);

		std::string listenHostname(std::string line);

	private: 
		std::map<std::string, uint16_t> _hostAddress;
		// std::ifstream& _fs;
};

#endif