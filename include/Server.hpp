#ifndef SERVER_HPP
#define SERVER_HPP

#include <ConfigServer.hpp>
#include <ServerListenFD.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <memory>

using namespace std;

class Server
{
	public:
    Server(ConfigServer &config);
    void    createListeners(vector<unique_ptr<Server>> &servers);
    ~Server() {}
	vector<int> _listeners; // ip/port
	
    ConfigServer &_config;
	private:
    
	
    // bool setupSocket();
    // void closeSocket();
};
using ServerList = vector<unique_ptr<Server>>;




#endif