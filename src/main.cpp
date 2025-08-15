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


namespace {
    int  runWebServer(int argc, char *argv[]);
    void setupEnvironment();
    void setupSignalHandlers();
    void configureServer(int argc, char *argv[]);
}

void sigint_handler(int signum)
{
    Logger::log(INFO, "SIGINT received, stopping webserver");
    g_signal_status = signum;
}
// #include <fcntl.h>
int main(int argc, char *argv[])
{
    try
    {
        return runWebServer(argc, argv);
    }
    catch (const Logger::ErrorLogExit&)
    {
        Logger::log(ERROR, "Logger exit triggered");
    }
    catch (const exception &e)
    {
        Logger::log(ERROR, "An error occurred: ", e.what());
    }
    catch(...)
    {
        Logger::log(ERROR, "An error occurred");
    }
    return EXIT_FAILURE;
}

namespace
{
    int runWebServer(int argc, char *argv[])
    {
        setupEnvironment();
        setupSignalHandlers();
        configureServer(argc, argv);
        RunServers::setupEpoll();
        RunServers::runServers();
        return EXIT_SUCCESS;
    }

    void setupEnvironment()
    {
        RunServers::getExecutableDirectory();
        Logger::initialize(LOG);
        atexit(FileDescriptor::cleanupFD);
    }

    void setupSignalHandlers()
    {
        if (signal(SIGINT, &sigint_handler))
            Logger::logExit(ERROR, "Failed to set SIGINT handler: ", strerror(errno));
    }

    void configureServer(int argc, char *argv[])
    {
        const char *confFile = (argc >= 2) ? argv[1] : DEFAULT_CONFIG;
        if (argc == 3)
        {
            try
            {
                RunServers::setClientBufferSize(stoullSafe(argv[2]));
            }
            catch(const std::exception& e)
            {
                Logger::logExit(ERROR, "Failed to set client buffer size: ", e.what());
            }
        }
        Parsing configFile(confFile);
        RunServers::createServers(configFile.getConfigs());
    }
}