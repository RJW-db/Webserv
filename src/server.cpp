#include <Webserv.hpp>
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
#include <sys/epoll.h>
#endif


Server::Server(tmp_t *serverConf)
{
	ServerListenFD listenerFD(serverConf->port);
	_serverName = serverConf->hostname;
	_listener = listenerFD.getFD();
}


Server::~Server()
{
	close(_listener);
}


int Server::make_socket_non_blocking(int sfd)
{
	int currentFlags = fcntl(sfd, F_GETFL, 0);
	if (currentFlags == -1)
	{
		std::cerr << "fcntl: " << strerror(errno);
		return -1;
	}

	currentFlags |= O_NONBLOCK;
	int fcntlResult = fcntl(sfd, F_SETFL, currentFlags);
	if (fcntlResult == -1)
	{
		std::cerr << "fcntl: " << strerror(errno);
		return -1;
	}
	return 0;
}