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
volatile sig_atomic_t g_signal_status = 0;

#include <sys/stat.h>
// Static Functions
static void examples(void);

void sigint_handler(int signum)
{
    std::cout << "sigint received stopping webserver" << std::endl;
    g_signal_status = signum;
}
#include <fcntl.h>
int main(int argc, char *argv[])
{

    FileDescriptor::initializeAtexit();
    if (signal(SIGINT, &sigint_handler))
    {
        std::cerr << "Error setting up signal handler: " << strerror(errno) << std::endl;
        // return 1;
    }
    string confFile = "config/default.conf";
    if (argc >= 2)
        confFile = argv[1];
    if (argc == 3)
        RunServers::setClientBufferSize(stoullSafe(argv[2]));
    Parsing test(confFile.data());
    // test.printAll();
    RunServers::createServers(test.getConfigs());
    RunServers::runServers();
    
    RunServers::cleanupServer(); // does nothing for now
    // FileDescriptor::cleanupFD();
    // examples();
    return 0;
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
