#include <sys/stat.h>
#include <unistd.h>
#include "FileDescriptor.hpp"
#include "RunServer.hpp"
#include "Parsing.hpp"
#include "Logger.hpp"
volatile sig_atomic_t g_signal_status = 0;
namespace
{
    constexpr char LOG_DIR[] = "log";
    constexpr char LOG[] = "webserv.log";
    constexpr char DEFAULT_CONFIG[] = "config/default.conf";

    int  runWebServer(int argc, char *argv[]);
    void setupEnvironment();
    void setupSignalHandlers();
    void sigintHandler(int signum);
    void configureServer(int argc, char *argv[]);
}

int main(int argc, char *argv[])
{
    try {
        return runWebServer(argc, argv);
    }
    catch (const Logger::ErrorLogExit&) {
        Logger::log(ERROR, "Logger exit triggered");
    }
    catch (const exception &e) {
        Logger::log(ERROR, "An error occurred: ", e.what());
    }
    catch (...) {
        Logger::log(ERROR, "An error occurred");
    }
    RunServers::cleanupEpoll();
    return EXIT_FAILURE;
}

namespace
{
    int runWebServer(int argc, char *argv[])
    {
        setupSignalHandlers();
        setupEnvironment();
        configureServer(argc, argv);
        RunServers::setupEpoll();
        RunServers::runServers();
        return EXIT_SUCCESS;
    }

    void setupEnvironment()
    {
        initRandomSeed();
        RunServers::getExecutableDirectory();
        Logger::initialize(LOG_DIR, LOG);
        if (FD_LIMIT <= 5)
            Logger::logExit(ERROR, "FD_LIMIT is set to an invalid value: ", FD_LIMIT,
                        "It must be between 6 and 65536. Please recompile with a valid FD_LIMIT.");
        atexit(FileDescriptor::cleanupAllFD);
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
        Parsing configFile(confFile);
        RunServers::createServers(configFile.getConfigs());
    }
}
