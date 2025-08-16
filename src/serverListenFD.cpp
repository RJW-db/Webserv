#include <RunServer.hpp>
#include "Logger.hpp"
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
	createListenerSocket();
}

ServerListenFD::~ServerListenFD()
{
}

int	ServerListenFD::getFD() const
{
	return (_listener);
}

void ServerListenFD::createListenerSocket()
{
	struct addrinfo *serverInfo = getServerAddrinfo();
	bindToSocket(serverInfo);
	freeaddrinfo(serverInfo);
	if (RunServers::makeSocketNonBlocking(_listener) == false)
		Logger::logExit(ERROR, "Server create_listener_socket: makeSocketNonBlocking failed");

	if (listen(_listener, SOMAXCONN) == -1)
		Logger::logExit(ERROR, "Server create_listener_socket: listen failed");
}

struct addrinfo* ServerListenFD::getServerAddrinfo(void)
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
		Logger::logExit(ERROR, "Server getaddrinfo: ", gai_strerror(errHndl));
	return server;
}

void ServerListenFD::bindToSocket(struct addrinfo *server)
{
	struct addrinfo *p;
	for (p = server; p != NULL; p = p->ai_next)
	{
		_listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		if (_listener < 0)
			continue;

		int reUse = 1;
		setsockopt(_listener, SOL_SOCKET, SO_REUSEADDR, &reUse, sizeof(int));
		if (bind(_listener, p->ai_addr, p->ai_addrlen) == -1)
		{
			if (FileDescriptor::safeCloseFD(_listener) == false)
				Logger::logExit(FATAL, "FileDescriptor::safeCloseFD: Attempted to close a file descriptor that is not in the vector: ", _listener);
			continue;
		}
		FileDescriptor::setFD(_listener);
		Logger::log(INFO, "Server created         listenFD:", _listener, " Successfuly bound to ", _hostName, ":", _port);
		RunServers::setEpollEvents(_listener, EPOLL_CTL_ADD, EPOLLIN);
		return;
	}
	freeaddrinfo(server);
	Logger::logExit(ERROR, "Server bindToSocket: ", strerror(errno));
}


