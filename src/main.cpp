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


// Static Functions
static void examples(void);
static void serverTest(void);
static void parsingtest(void);
static void openDir(void);
// static void customHandler(int signum);

int main()
{
	// signal(SIGINT, customHandler);
	examples();
	openDir();

	serverTest();
	parsingtest();


	// httpRequestLogger(std::string("Syntax error in request: GET /favicon.ico HTTP/1.1\r\n"));
    return 0;
}


static void examples(void)
{
    // poll_usages();
    // epoll_usage();
    // getaddrinfo_usage();
    // server();
}



static void serverTest(void)
{
	// FileDescriptor	fds;
    // ServerList servers;

	// servers.push_back(make_unique<Server>(make_unique<tmp_t>(tmp_t{"Alpha", "8080"}).get()));
	// // servers.push_back(make_unique<Server>(make_unique<tmp_t>(tmp_t{"Beta", "6789"}).get()));

	// Server::epollInit(servers);
    // Server::runServers(servers, fds);

	// std::vector<tmp_t> servConf;
	// servConf.push_back((tmp_t){"Alpha", "8080"});
}

static void parsingtest(void)
{
	// ConfigServer sam;
	Parsing sam("config/default.conf");
}

static void openDir(void)
{
	// string path = "rde-brui";
	// cout << "folder \"" << Server::directoryCheck(path) << "\" exists" << endl;
}

// static void customHandler(int signum)
// {
// 	g_signal_status = signum;
// }
