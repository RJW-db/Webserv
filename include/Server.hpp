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

class Server : public AconfigServ
{
	public:
    	Server(const AconfigServ &config);
		Server(const Server &other);

		Server &operator=(const Server &other);
    	// Server(const Server &other);

		void addListener(int listener);
    	~Server() {}

    	static void    createListeners(vector<unique_ptr<Server>> &servers);

		vector<int> &getListeners();

	private:
		vector<int> _listeners; // ip/port
};
using ServerList = vector<unique_ptr<Server>>;




#endif