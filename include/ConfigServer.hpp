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

using namespace std;

class Aconfig
{
	public:
		virtual ~Aconfig() = default; // Ensures proper cleanup of derived classes

		string error_page(string line, bool &findColon);
		string root(string line, bool &findColon);
		string ClientMaxBodysize(string line, bool &findColon);
		size_t clientBodySize;

	protected:
		Aconfig() = default;
		map<uint16_t, string> ErrorCodesWithPage;
		vector<uint16_t> ErrorCodesWithoutPage;
		string _root;
	private :
};

class ConfigServer : public Aconfig
{
	public:
		ConfigServer();
		// ConfigServer(std::fstream &fs);
		ConfigServer(const ConfigServer &other);
		virtual ~ConfigServer();


		ConfigServer &operator=(const ConfigServer &other);

		
		string listenHostname(string line, bool &findColon);
		// string error_page(string line, bool &findColon) override;
        // string root(string line, bool &findColon) override;
        // string ClientMaxBodysize(string line, bool &findColon) override;
		// string error_page(string line, bool &findColon);
		// void	addHostPort(string line);

		unordered_map<string, sockaddr> _hostAddress;
		// map<uint16_t, string> ErrorCodesWithPage;
		private: 
			// vector<uint16_t> ErrorCodesWithoutPage;
};


class Location
{
	public :
		Location();
		Location(const Location &other);
		Location operator=(const Location &other);
		~Location();


};

string ftSkipspace(string &line);


#endif