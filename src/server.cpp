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

bool Server::_isRunning = true;
int Server::_epfd = -1;
std::array<struct epoll_event, FD_LIMIT> Server::_events;

Server::Server(tmp_t *serverConf)
{
	ServerListenFD listenerFD(serverConf->port);
	_serverName = serverConf->hostname;
	_listener = listenerFD.getFD();
}


Server::~Server()
{
	close(_listener);
	close(_epfd);
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

int Server::epollInit(ServerList &servers)
{
    _epfd = epoll_create(FD_LIMIT); // parameter must be bigger than 0, rtfm
    if (_epfd == -1)
    {
        std::cerr << "Server epoll_create: " << strerror(errno);
        return -1;
    }

	for (const std::unique_ptr<Server>& server : servers)
	{
		struct epoll_event current_event;
		current_event.data.fd = server->_listener;
		current_event.events = EPOLLIN | EPOLLET;
		if (epoll_ctl(_epfd, EPOLL_CTL_ADD, server->_listener, &current_event) == -1)
		{
			std::cerr << "Server epoll_ctl: " << strerror(errno) << std::endl;
			close(_epfd);
			return -1;
		}
	}
	return 0;
}