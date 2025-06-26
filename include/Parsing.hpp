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
#include <Server.hpp>
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
		vector<Server> &getServers(void) const;
		void printAll() const;

		vector<ConfigServer> &getConfigs();
		
	private:
		template <typename T>
		void readBlock(T &block, 
			const map<string, bool (T::*)(string&)> &cmds, 
			const map<string, bool (T::*)(string &)> &whileCmds);
		
		template <typename T>
		void LocationCheck(string &line, T &block, bool &validSyntax);
		void ServerCheck();

		template <typename T>
		void cmdCheck(string &line, T &block, const pair<const string, bool (T::*)(string &)> &cmd);
		template <typename T>
		void whileCmdCheck(string &line, T &block, const pair<const string, bool (T::*)(string &)> &cmd);

		bool    runReadblock(void);
		template <typename T>
		void skipLine(string &line, bool forceSkip, T &curConf, bool skipSpace);
		// void foundServer(std::fstream &fs, std::string line);

		// ConfigServer	*_confServers;
		// uint8_t 		_countServ;
		vector<ConfigServer> _configs;

		map<int, string> _lines;
		bool validSyntax;
		// vector<Server> _servers;
};


#endif
