#include <RunServer.hpp>
#include <iostream>

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // close
#include <stdlib.h>	// callod
#ifdef __linux__
# include <sys/epoll.h>
#endif

ServerListenFD::ServerListenFD(const char *port, const char *hostName)
: _port(port), _hostName(hostName)
{
	create_listener_socket();
}

ServerListenFD::~ServerListenFD()
{
}

int	ServerListenFD::getFD() const
{
	return (_listener);
}


int ServerListenFD::create_listener_socket()
{
	struct addrinfo *serverInfo = get_server_addrinfo();
	if (serverInfo == NULL)
	{
		return -1;
	}

	_listener = bind_to_socket(serverInfo);
	freeaddrinfo(serverInfo);
	if (_listener == -1)
	{
		return -1;
	}
	if (RunServers::make_socket_non_blocking(_listener) == false)
	{
		return -1;
	}
	// TODO test with 1~5 maximum pending queue of people connecting
	if (listen(_listener, SOMAXCONN) == -1)
	{
		cerr << "Server listen: " << strerror(errno);
		return -1;
	}
	return _listener;
}

struct addrinfo* ServerListenFD::get_server_addrinfo(void)
{
	struct addrinfo  serverSetup;
	struct addrinfo *server;
	int              errHndl;

	memset(&serverSetup, 0, sizeof(serverSetup));
	serverSetup.ai_family = AF_UNSPEC;     // Use IPv4 or IPv6
	serverSetup.ai_socktype = SOCK_STREAM; // TCP stream sockets
	serverSetup.ai_flags = AI_PASSIVE;     // Use my IP

	errHndl = getaddrinfo(_hostName, _port, &serverSetup, &server);
	if (errHndl != 0)
	{
		cerr << "Server getaddrinfo: " << gai_strerror(errHndl)
				<< endl;
		return NULL;
	}
	return server;
}

int ServerListenFD::bind_to_socket(struct addrinfo *server)
{
	struct addrinfo *p;
	for (p = server; p != NULL; p = p->ai_next)
	{
		_listener =
			socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (_listener < 0)
		{
			continue;
		}
		int reUse = 1;
		setsockopt(_listener, SOL_SOCKET, SO_REUSEADDR, &reUse, sizeof(int));
		if (bind(_listener, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(_listener);
			continue;
		}
		return _listener;
	}
	// std::cout << _port << " " << _hostName  << std::endl;
	cerr << "Server bind_to_socket: " << strerror(errno);
	return -1;
}


