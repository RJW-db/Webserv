#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "Client.hpp"

#ifndef TERMINAL_DEBUG
# define TERMINAL_DEBUG true
#endif

using namespace std;
constexpr char LOG_ERROR[] = "Logger error occurred\n";


enum LogLevel : uint8_t
{
    CHILD_INFO = 0, // cout
    INFO       = 1, // cout
    DEBUG      = 2, // cerr
    WARN       = 3, // cerr
    ERROR      = 4, // cerr
    FATAL      = 5  // cerr
};

class Logger {
private:
    static int _logFd;

    static string getTimeStamp();
    static string logLevelToString(uint8_t level, bool useColors);

public:
    static void initialize(const char *logPath);

    template<typename... Args>
    static void log(int level, Args&&... args);

    template<typename... Args>
    static void logExit(int level, Args&&... args);

    template<typename... Args>
    static void log(int level, Client &client, Args&&... args);

    class ErrorLogExit : public std::exception
    {
    public:
        explicit ErrorLogExit() noexcept {}
        const char *what() const noexcept override {
            return "Exiting";
        }
    };
};

template<typename... Args>
void Logger::log(int level, Args&&... args)
{
    try
    {
        ostringstream oss;
        (oss << ... << args);
        string rawMessage = oss.str();
        
        string timeStamp = getTimeStamp();
        int lvlStr = level == CHILD_INFO ? level + 1 : level;
        string levelStr = logLevelToString(lvlStr, LOGGER);

        ostringstream logMsg;
        logMsg << timeStamp << levelStr << escapeSpecialChars(rawMessage, LOGGER) << "\n";
        string logStr = logMsg.str();

        if (_logFd != -1 && write(_logFd, logStr.c_str(), logStr.length()) == -1)
        {
            cerr << "Error writing to log file: " << strerror(errno) << endl;
            _logFd = -1;
        }

        if (level >= DEBUG || (TERMINAL_DEBUG == true && level == INFO))
        {
            levelStr = logLevelToString(lvlStr, TERMINAL);
            ostream& output = (level == INFO) ? cout : cerr;
            output << timeStamp + levelStr + escapeSpecialChars(rawMessage, TERMINAL) + "\n";
        }
    }
    catch (...)
    {
        if (_logFd != -1)
            write(_logFd, LOG_ERROR, sizeof(LOG_ERROR) - 1);
        write(STDERR_FILENO, LOG_ERROR, sizeof(LOG_ERROR) - 1);
    }
}

template<typename... Args>
void Logger::logExit(int level, Args&&... args)
{
    log(level, std::forward<Args>(args)...);
    throw Logger::ErrorLogExit();
    // exit(EXIT_FAILURE);
}

template<typename... Args>
void Logger::log(int level, Client &client, Args&&... args)
{
    try
    {
        auto portHostMap = client._usedServer->getPortHost().begin();
        string portHostInfo = portHostMap->second + ":" + portHostMap->first;
        // string portHostInfo = "255.255.255.255:65535";
        
        // max size 255.255.255.255:65535
        if (portHostInfo.length() < 21)
            portHostInfo.append(21 - portHostInfo.length(), ' ');
        log(level, portHostInfo, "  clientFD:", client._fd, "  ", args...);
        // log(level, client._usedServer->getServerName(), "  FD:", client._fd, "  ", args...);
    }
    catch(...)
    {
        if (_logFd != -1)
            write(_logFd, LOG_ERROR, sizeof(LOG_ERROR) - 1);
        write(STDERR_FILENO, LOG_ERROR, sizeof(LOG_ERROR) - 1);
    }
}
#endif
