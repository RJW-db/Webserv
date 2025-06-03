#ifndef CONFIGSERVER_HPP
#define CONFIGSERVER_HPP

#include <unordered_map>
#include <map>
#include <string>
#include <stdint.h>	// uint16_t, cstdint doesn't exist in std=98
#include <fstream>
#include <sys/socket.h> //sockaddr
#include <netinet/in.h> //scokaddr_in
#include <iostream>
#include <vector>
#include <Location.hpp>

using namespace std;

class ConfigServer : public Aconfig
{
	public:
		ConfigServer();
		// ConfigServer(std::fstream &fs);
		ConfigServer(const ConfigServer &other);
		virtual ~ConfigServer();


		ConfigServer &operator=(const ConfigServer &other);

		
		bool listenHostname(string &line);
		bool serverName(string &line);

		unordered_map<string, sockaddr> _hostAddress;
		// map<uint16_t, string> ErrorCodesWithPage;
		vector<Location> _locations;
		string _serverName; // if not found acts as default
		private:
			uint32_t convertIpBinary(string ip);

			// vector<uint16_t> ErrorCodesWithoutPage;
};




// string ftSkipspace(string &line);
void ftSkipspace(string &line);


#endif