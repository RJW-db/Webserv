#ifndef CONFIGSERVER_HPP
#define CONFIGSERVER_HPP

#include <unordered_map>
#include <string>
#include <stdint.h>	// uint16_t, cstdint doesn't exist in std=98
#include <fstream>
#include <sys/socket.h> //sockaddr
#include <netinet/in.h> //scokaddr_in
#include <iostream>

using namespace std;
class ConfigServer
{
	public:
		ConfigServer();
		// ConfigServer(std::fstream &fs);
		ConfigServer(const ConfigServer &other);
		~ConfigServer();


		ConfigServer &operator=(const ConfigServer &other);

		
		string listenHostname(string line);
		// void	addHostPort(string line);

		unordered_map<string, sockaddr> _hostAddress;
	private: 
};

#endif