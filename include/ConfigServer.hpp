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

		
		string listenHostname(string line, bool &findColon);
		string serverName(string line, bool &findColon);
		// string error_page(string line, bool &findColon) override;
        // string root(string line, bool &findColon) override;
        // string ClientMaxBodysize(string line, bool &findColon) override;
		// string error_page(string line, bool &findColon);
		// void	addHostPort(string line);

		unordered_map<string, sockaddr> _hostAddress;
		// map<uint16_t, string> ErrorCodesWithPage;
		vector<Location> _locations;
		string _serverName; // if not found acts as default
		private: 
			// vector<uint16_t> ErrorCodesWithoutPage;
};




// string ftSkipspace(string &line);
void ftSkipspace(string &line);


#endif