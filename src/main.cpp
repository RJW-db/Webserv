#include <sys/stat.h>
#include <RunServer.hpp>
#include <Parsing.hpp>
#include <FileDescriptor.hpp>
#include <Logger.hpp>

constexpr char LOG_DIR[] = "log";
constexpr char LOG[] = "webserv.log";
constexpr char DEFAULT_CONFIG[] = "config/default.conf";
volatile sig_atomic_t g_signal_status = 0;


namespace {
    int  runWebServer(int argc, char *argv[]);
    void setupEnvironment();
    void setupSignalHandlers();
    void sigintHandler(int signum);
    void configureServer(int argc, char *argv[]);
}

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
        Logger::initialize(LOG_DIR, LOG);
        atexit(FileDescriptor::cleanupFD);
    }

    void setupSignalHandlers()
    {
        if (signal(SIGINT, &sigintHandler) == SIG_ERR)
            Logger::logExit(ERROR, "Signal handler error", '-', "SIGINT failed", strerror(errno));
    }

    void sigintHandler(int signum)
    {
        Logger::log(INFO, "SIGINT received, stopping webserver");
        g_signal_status = signum;
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
                Logger::logExit(ERROR, "Config error", '-', "Client buffer size failed", e.what());
            }
        }
        Parsing configFile(confFile);
        RunServers::createServers(configFile.getConfigs());
    }
}
