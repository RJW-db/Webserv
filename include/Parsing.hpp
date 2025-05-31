#ifndef PARSING_HPP
# define PARSING_HPP

#include <cstring>
#include <string>
#include <ConfigServer.hpp>
#include <stdint.h>	// uint16_t, cstdint doesn't exist in std=98
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <array>
using namespace std;

// typedef struct configServer
// {
// 	vector<sockaddr_in> hostnamePort;
// 	std::string serverName;
// }	configServer_t;

class Parsing
{
	public:
		Parsing(const char *input);
		~Parsing();
		vector<ConfigServer> _configs;

	private:
		template <typename T>
		void readBlock(T &block, 
			const map<string, bool (T::*)(string&)> &cmds, 
			const map<string, string (T::*)(string, bool &)> &whileCmds);
		bool    runReadblock(void);
		string skipLine(string &line, bool forceSkip);
		// void foundServer(std::fstream &fs, std::string line);

		// ConfigServer	*_confServers;
		// uint8_t 		_countServ;
		map<int, string> _lines;
		bool validSyntax;
};


#endif
