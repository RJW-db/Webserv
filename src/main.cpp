#include <RunServer.hpp>
#include <Parsing.hpp>
#include <FileDescriptor.hpp>
#include <help.hpp>
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
	// FileDescriptor	fds;
    // RunServers servers;

	// // servers.push_back(make_unique<RunServers>(make_unique<tmp_t>(tmp_t{"Alpha", "8080"}).get()));
	// // servers.push_back(make_unique<Server>(make_unique<tmp_t>(tmp_t{"Beta", "6789"}).get()));
	// Parsing test("config/default.conf");
	// for (ConfigServer config : test._configs)
	// {
	// 	Server server(config);		
	// }
	// // RunServers servers(test.getServers());
	// // RunServers::epollInit(servers);
    // // RunServers::runServers(servers, fds);

	// // std::vector<tmp_t> servConf;
	// // servConf.push_back((tmp_t){"Alpha", "8080"});



	FileDescriptor	fds;
    ServerList servers;

	servers.push_back(make_unique<RunServers>(make_unique<tmp_t>(tmp_t{"Alpha", "8080"}).get()));
	// servers.push_back(make_unique<Server>(make_unique<tmp_t>(tmp_t{"Beta", "6789"}).get()));

	RunServers::epollInit(servers);
    RunServers::runServers(servers, fds);

	std::vector<tmp_t> servConf;
	servConf.push_back((tmp_t){"Alpha", "8080"});

}

static void parsingtest(void)
{
	// ConfigServer sam;
	try
	{
		Parsing sam("config/default.conf");
		for (const auto& entry : sam._configs[0]._hostAddress) {
			const std::string& key = entry.first;
			const sockaddr& addr = entry.second;
	
			if (addr.sa_family == AF_INET) { // IPv4
				const sockaddr_in* addr_in = reinterpret_cast<const sockaddr_in*>(&addr);
				std::string ip = inet_ntoa(addr_in->sin_addr); // Convert IP to string
				uint16_t port = ntohs(addr_in->sin_port);      // Convert port to host byte order
				cout << "port" << port << endl;
	
				std::cout << "Key: " << key << ", IP: " << ip << ", Port: " << port << std::endl;
			} else {
				std::cout << "Key: " << key << ", Unsupported address family" << std::endl;
			}
		}
		
		std::cout << "autoindex location:" << static_cast<int>(sam._configs[0]._locations[0]._autoIndex) << std::endl;
		std::cout << sam._configs[0]._root << std::endl;
		std::cout << "clientBodySize: " << sam._configs[0]._clientBodySize << std::endl;
		std::cout << sam._configs[0]._returnRedirect.first << " " << sam._configs[0]._returnRedirect.second << std::endl;
		std::cout << "methods location: " << sam._configs[0]._locations[0]._methods[0] << std::endl;
		// std::cout << sam._configs[0]._locations[0]._indexPage[1] << std::endl;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	// cout << "methods :" << sam._configs[0].locations[0]._methods[0] << endl;
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

bool directoryCheck(string &path)
{
    DIR *d = opendir(path.c_str());	// path = rde-brui
    if (d == NULL) {
        perror("opendir");
        return false;
    }

    // struct dirent *directoryEntry;
    // while ((directoryEntry = readdir(d)) != NULL) {
    //     printf("%s\n", directoryEntry->d_name);
    //     if (string(directoryEntry->d_name) == path)
    //     {
    //         closedir(d);
    //         return (true);
    //     }
    // }
    
    closedir(d);
    return (true);
    // return (false);
}