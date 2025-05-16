#ifndef PARSING_HPP
# define PARSING_HPP

#include <cstring>
#include <string>
#include <ConfigServer.hpp>
#include <stdint.h>	// uint16_t, cstdint doesn't exist in std=98
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
using namespace std;

typedef struct configServer
{
	vector<sockaddr_in> hostnamePort;
	std::string serverName;
}	configServer_t;

class Parsing
{
	public:
		Parsing(const char *input);
		~Parsing();

	private:
		void readServer();

		sockaddr_in listenHostname();


		// void foundServer(std::fstream &fs, std::string line);

		vector<configServer_t> _configs;
		// ConfigServer	*_confServers;
		// uint8_t 		_countServ;
		std::vector<std::string> _lines;

};


#endif
