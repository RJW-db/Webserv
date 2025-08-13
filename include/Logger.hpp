#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "Client.hpp"

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
    static ofstream _logFile;

    static string getTimeStamp();
    static string logLevelToString(uint8_t level);
    static string escapeSpecialCharsRaw(const string &input);


public:
    static void initialize(const string& logPath);
    static void cleanup();
    
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
        string levelStr = logLevelToString(level < CHILD_DEBUG ? level : level - 5);
        
        if (level < CHILD_DEBUG)
            cout << timeStamp + levelStr + escapeSpecialChars(rawMessage, TERMINAL) << endl;
        
        if (_logFile.is_open())
        {
            _logFile << timeStamp + levelStr + escapeSpecialChars(rawMessage, LOGGER) << endl;
            _logFile.flush();
        }
    }
    catch (...)
    {
        try
        {
            if (level < CHILD_DEBUG)
                cerr << LOG_ERROR;
        
            if (_logFile.is_open())
            {
                _logFile << LOG_ERROR;
                _logFile.flush();
            }
        } catch (...) {
            write(STDERR_FILENO, LOG_ERROR, sizeof(LOG_ERROR) - 1);
        }
    }
}

template<typename... Args>
void Logger::log(int level, Client &client, Args&&... args)
{
    try
    {
        auto portHostMap = client._usedServer->getPortHost().begin();
        string portHostInfo = portHostMap->second + ":" + portHostMap->first;
        log(level, portHostInfo, "  FD:", client._fd, "  ", args...);
        log(level, client._usedServer->getServerName(), "  FD:", client._fd, "  ", args...);
    }
    catch(...)
    {
        try
        {
            if (level < CHILD_DEBUG)
                cerr << LOG_ERROR;
        
            if (_logFile.is_open())
            {
                _logFile << LOG_ERROR;
                _logFile.flush();
            }
        }
        catch (...)
        {
            write(STDERR_FILENO, LOG_ERROR, sizeof(LOG_ERROR) - 1);
        }
    }
}
#endif
