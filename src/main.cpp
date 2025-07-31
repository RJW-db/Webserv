#include <RunServer.hpp>
#include <Parsing.hpp>
#include <FileDescriptor.hpp>

volatile sig_atomic_t g_signal_status = 0;

#include <sys/stat.h>
// Static Functions
// static void examples(void);

void sigint_handler(int signum)
{
    std::cout << "sigint received stopping webserver" << std::endl;
    g_signal_status = signum;
}
#include <fcntl.h>
// void handleCgi(Client &client);
int main(int argc, char *argv[])
{
    // Client client(1);
    // handleCgi(client);
    // exit(0);
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



// static void examples(void)
// {
    // poll_usages();
    // epoll_usage();
    // getaddrinfo_usage();conflicts
    // server();
// }

// static void customHandler(int signum)
// {
// 	g_signal_status = signum;
// }
