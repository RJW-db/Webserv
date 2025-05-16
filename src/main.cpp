#include <Webserv.hpp>
#include <Parsing.hpp>
#include <FileDescriptor.hpp>

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

#include <iostream>
#include <memory>
#include <vector>


// int              create_listener_socket(void);
// struct addrinfo *get_server_addrinfo(void);
// int              bind_to_socket(struct addrinfo *server);


int main()
{
    // poll_usages();
    // epoll_usage();
    // getaddrinfo_usage();
    // server();

	// Parsing sam("./config/default.conf");

	// exit(0);
	
	
	
	
	FileDescriptor	fds;
	tmp_t			serverConfig[2];
	serverConfig[0] = (tmp_t){"Alpha", "8080"};
	// serverConfig[1] = (tmp_t){"Beta", "6789"};

    ServerList servers;
    size_t amount_servers = 1;
	for (size_t i = 0; i < amount_servers; ++i) {
		servers.push_back(make_unique<Server>(serverConfig + i));
	}

	Server::epollInit(servers);
    Server::runServers(servers, fds);
    return 0;
}

