#include <RunServer.hpp>
#include <Parsing.hpp>
#include <FileDescriptor.hpp>
// #include <arpa/inet.h>
// #include <cstring>
// #include <errno.h>
// #include <fcntl.h>
// #include <string.h>
// #include <netdb.h>
// #include <netinet/in.h>
// #include <poll.h>
// #include <stdio.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <sys/types.h>
// #include <unistd.h> // close
// #include <stdlib.h>	// callod
// #ifdef __linux__
// #include <sys/epoll.h>
// #endif

// #include <dirent.h>

// #include <iostream>
// #include <memory>
// #include <vector>
#include <signal.h>
volatile sig_atomic_t g_signal_status = 0;


// Static Functions
static void examples(void);

int main()
{
	Parsing test("config/default.conf");
	test.printAll();
	RunServers::createServers(test.getConfigs());

	RunServers::epollInit();
    RunServers::runServers();
    return 0;
	// examples();
}


static void examples(void)
{
    // poll_usages();
    // epoll_usage();
    // getaddrinfo_usage();conflicts
    // server();
}

// static void customHandler(int signum)
// {
// 	g_signal_status = signum;
// }
