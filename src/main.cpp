#include <RunServer.hpp>
#include <Parsing.hpp>
#include <FileDescriptor.hpp>
#include <Logger.hpp>

constexpr char LOG[] = "logs/webserv.log";
constexpr char DEFAULT_CONFIG[] = "config/default.conf";
volatile sig_atomic_t g_signal_status = 0;

#include <sys/stat.h>
// Static Functions
// static void examples(void);

void sigint_handler(int signum)
{
    Logger::log(INFO, "SIGINT received, stopping webserver");
    g_signal_status = signum;
}
#include <fcntl.h>
int main(int argc, char *argv[])
{
    // atexit(Logger::cleanup);
    atexit(FileDescriptor::cleanupFD);
    RunServers::getExecutableDirectory();
    Logger::initialize(LOG);
    if (signal(SIGINT, &sigint_handler))
    {
        Logger::logExit(ERROR, "Failed to set SIGINT handler: ", strerror(errno));
        return 1;
    }
    const char *confFile = (argc >= 2) ? argv[1] : DEFAULT_CONFIG;
    if (argc == 3)
        RunServers::setClientBufferSize(stoullSafe(argv[2]));

    
    Parsing test(confFile);
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
