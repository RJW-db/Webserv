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
          void readBlock(T &block, const std::array<std::string, 3> &cmds_strings, 
					string (T::*funcs[])(string, bool &),
                    const std::array<std::string, 1> &whileCmdsStrings,
                    string (T::*whileFuncs[])(string, bool &));

          // void foundServer(std::fstream &fs, std::string line);

          // ConfigServer	*_confServers;
          // uint8_t 		_countServ;
          std::vector<std::string> _lines;

};


#endif
