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
    DEBUG = 0,
    INFO  = 1,
    WARN  = 2,
    ERROR = 3,
    FATAL = 4,
    CHILD_DEBUG = 5,
    CHILD_INFO  = 6,
    CHILD_WARN  = 7,
    CHILD_ERROR = 8,
    CHILD_FATAL = 9
};

class Logger {
private:
    static int _logFd;

    static string getTimeStamp();
    static string logLevelToString(uint8_t level, bool useColors);

public:
    static void initialize(const string& logPath);
    
    template<typename... Args>
    static void log(int level, Args&&... args);
    
    // Client-specific logging - calls the general one
    template<typename... Args>
    static void log(int level, Client &client, Args&&... args);
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
        int lvlStr = level < CHILD_DEBUG ? level : level - 5;
        string levelStr = logLevelToString(lvlStr, LOGGER);

        ostringstream logMsg;
        logMsg << timeStamp << levelStr << escapeSpecialChars(rawMessage, LOGGER) << "\n";
        string logStr = logMsg.str();

        if (_logFd != -1 && write(_logFd, logStr.c_str(), logStr.length()) == -1)
        {
            cerr << "Error writing to log file: " << strerror(errno) << endl;
            _logFd = -1;  // Mark as closed to prevent future write attempts
        }

        if (TERMINAL_DEBUG && level < CHILD_DEBUG)
        {
            levelStr = logLevelToString(lvlStr, TERMINAL);
            ostream& output = (level >= WARN && level <= FATAL) ? cerr : cout;
            output << timeStamp + levelStr + escapeSpecialChars(rawMessage, TERMINAL) + "\n";
        }
        
    }
    catch (...)
    {
        if (_logFd != -1)
            write(_logFd, LOG_ERROR, sizeof(LOG_ERROR) - 1);
        if (level < CHILD_DEBUG)
            write(STDERR_FILENO, LOG_ERROR, sizeof(LOG_ERROR) - 1);
    }
}

template<typename... Args>
void Logger::log(int level, Client &client, Args&&... args)
{
    try
    {
        auto portHostMap = client._usedServer->getPortHost().begin();
        string portHostInfo = portHostMap->second + ":" + portHostMap->first;
        
        // max size 255.255.255.255:65535
        if (portHostInfo.length() < 20) {
            portHostInfo.append(20 - portHostInfo.length(), ' ');
        } else if (portHostInfo.length() > 20) {
            portHostInfo = portHostInfo.substr(0, 20);  // Truncate if too long
        }
        portHostInfo += " ";
        // log(level, portHostInfo, "  FD:", client._fd, "  ", args...);
        log(level, portHostInfo, "  ClientFD:", client._fd, "  ", args...);
        // log(level, client._usedServer->getServerName(), "  FD:", client._fd, "  ", args...);
    }
    catch(...)
    {
        if (_logFd != -1)
            write(_logFd, LOG_ERROR, sizeof(LOG_ERROR) - 1);
        if (level < CHILD_DEBUG)
            write(STDERR_FILENO, LOG_ERROR, sizeof(LOG_ERROR) - 1);
    }
}
#endif
