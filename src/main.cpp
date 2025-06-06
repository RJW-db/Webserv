#include <RunServer.hpp>
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
using std::string;
void func(std::string_view part)
{
	// std::cout << part << std::endl;
	// string newstr = "destroy " + string(part);

}
int main()
{
	// signal(SIGINT, customHandler);
	// examples();
	// openDir();

	serverTest();
	// parsingtest();

	// std::cout << newstr << std::endl;
	// string something = "like a bird";
	// func(string_view(something).substr(5, 6));
	// std::cout << something << std::endl;

// string test = "okenondan";
// std::cout << test.find("non", 0) << std::endl;
	// string example = "  \t\tHost \t\t  : 10.10.3.26:8080";
	// size_t colonPos = example.find(':');
    // std::string key = example.substr(0, colonPos);

	// std::cout << escape_special_chars(key) << std::endl;
	// key.erase(0, key.find_first_not_of(" \t"));
	// std::cout << escape_special_chars(key) << std::endl;
    // key.erase(key.find_last_not_of(" \t") + 1);
	// std::cout << escape_special_chars(key) << std::endl;



	// httpRequestLogger(std::string("Syntax error in request: GET /favicon.ico HTTP/1.1\r\n"));
    return 0;
}


static void examples(void)
{
    // poll_usages();
    // epoll_usage();
    // getaddrinfo_usage();conflicts
    // server();
}



static void serverTest(void)
{

	Parsing test("config/default.conf");
	test.printAll();
	ServerList servers;

	// for (ConfigServer &config : test._configs)
	// {
	// 	servers.push_back(make_unique<Server>(Server(config)));
	// }
	// Server::createListeners(servers);
	RunServers::createServers(test._configs);
	FileDescriptor	fds;

	RunServers::epollInit();
    RunServers::runServers(fds);
}

// static void parsingtest(void)
// {
// 	// ConfigServer sam;
// 	try
// 	{
// 		Parsing sam("config/default.conf");
		

// 	}
// 	catch(const std::exception& e)
// 	{
// 		std::cerr << e.what() << '\n';
// 	}
// 	// cout << "methods :" << sam._configs[0].locations[0]._methods[0] << endl;
// }

static void openDir(void)
{
	// string path = "rde-brui";
	// cout << "folder \"" << Server::directoryCheck(path) << "\" exists" << endl;
}

// static void customHandler(int signum)
// {
// 	g_signal_status = signum;
// }
