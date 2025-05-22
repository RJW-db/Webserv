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

#include <dirent.h>

#include <iostream>
#include <memory>
#include <vector>
#include <signal.h>
volatile sig_atomic_t g_signal_status = 0;


// int              create_listener_socket(void);
// struct addrinfo *get_server_addrinfo(void);
// int              bind_to_socket(struct addrinfo *server);

void examples(void)
{
    // poll_usages();
    // epoll_usage();
    // getaddrinfo_usage();
    // server();
}

void customHandler(int signum)
{
	g_signal_status = signum;
}

int main()
{
	// signal(SIGINT, customHandler);

	
	// examples();
	// // ConfigServer sam;
	Parsing sam("config/default.conf");
	for (const auto& [errorCode, errorPage] : sam._configs[0].ErrorCodesWithPage)
	{
		std::cout << errorCode << ": " << errorPage << std::endl;
	}
	
	
	// FileDescriptor	fds;
	// tmp_t			serverConfig[2];
	// serverConfig[0] = (tmp_t){"Alpha", "8080"};
	// // serverConfig[1] = (tmp_t){"Beta", "6789"};

    // ServerList servers;
    // size_t amount_servers = 1;
	// for (size_t i = 0; i < amount_servers; ++i) {
	// 	servers.push_back(make_unique<Server>(serverConfig + i));
	// }

	// Server::epollInit(servers);
    // Server::runServers(servers, fds);



	// string path = "rde-brui";
	// cout << "folder \"" << Server::directoryCheck(path) << "\" exists" << endl;
    return 0;
}
