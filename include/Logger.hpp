#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <utils.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstring>
#include <unistd.h>
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
    IWARN      = 2, // cerr
    WARN       = 3, // cerr
    ERROR      = 4, // cerr
    FATAL      = 5, // cerr
    DEBUG      = 6  // cerr
};

class Logger {
private:
    static int _logFd;

    static string getTimeStamp();
    static string logLevelToString(uint8_t level, bool useColors);

    static inline string padRight(const string& s, size_t width)
    {
        return (s.size() < width) ? s + string(width - s.size(), ' ') : s;
    }

    // Left pad (for numbers, right-align)
    static inline string padLeft(const string& s, size_t width)
    {
        return (s.size() < width) ? string(width - s.size(), ' ') + s : s;
    }

    template<typename T>
    static string argToString(const T& arg);

    template<typename Tuple, size_t... Is>
    static string processArgsToString(const Tuple& tup, index_sequence<Is...>);

    template<typename Tuple, size_t... Is>
    static string processErrorArgsToString(const Tuple& tup, index_sequence<Is...>);
public:
    static void initialize(const string &logDir, const string &logFilename);
    static string initLogDirectory(const string &logDir);


    template<typename... Args>
    static void log(uint8_t level, Args&&... args);

    template<typename... Args>
    static void logExit(uint8_t level, Args&&... args);

    template<typename... Args>
    static void log(uint8_t level, Client &client, Args&&... args);

    class ErrorLogExit : public exception
    {
    public:
        explicit ErrorLogExit() noexcept {}
        const char *what() const noexcept override {
            return "Exiting";
        }
    };
};

// Helper to convert any type to string
template<typename T>
string Logger::argToString(const T& arg)
{
    ostringstream oss;
    oss << arg;
    return oss.str();
}

// Helper to process each argument by index
template<typename Tuple, size_t... Is>
string Logger::processArgsToString(const Tuple& tup, index_sequence<Is...>)
{
    string out;
    ((out += (
        Is == 0 ? Logger::padRight(Logger::argToString(get<Is>(tup)), 24) : // left-aligned message
        Is == 1 ? Logger::padLeft(Logger::argToString(get<Is>(tup)), 6) :   // right-aligned number/dash
        Is == 2 ? ':' + Logger::padRight(Logger::argToString(get<Is>(tup)), 14) : // left-aligned label/reason
        Logger::argToString(get<Is>(tup))                                       // rest (no padding)
    )), ...);
    return out;
}

// "ErrorType", "-", "Reason", "File"
template<typename Tuple, size_t... Is>
string Logger::processErrorArgsToString(const Tuple& tup, index_sequence<Is...>)
{
    string out;
    ((out += (
        Is == 0 ? Logger::padRight(Logger::argToString(get<Is>(tup)), 24) : // message
        Is == 1 ? Logger::padLeft(Logger::argToString(get<Is>(tup)), 6) :   // number (right-aligned)
        Is == 2 ? string(14, ' ') + Logger::argToString(get<Is>(tup)) :      // always 6 spaces before reason
        Logger::argToString(get<Is>(tup))                                      // rest (no padding)
    )), ...);
    return out;
}

template<typename... Args>
void Logger::log(uint8_t level, Args&&... args)
{
    try
    {
        string rawMessage;
        switch (level)
        {
            case CHILD_INFO:
            case INFO:
            case IWARN:
            {
                auto tup = forward_as_tuple(forward<Args>(args)...);
                rawMessage = processArgsToString(tup, index_sequence_for<Args...>{});
                // cout << rawMessage << endl; //testcout
                break;
            }
            case WARN:
            case ERROR:
            case FATAL:
            {
                auto tup = forward_as_tuple(forward<Args>(args)...);
                rawMessage = processErrorArgsToString(tup, index_sequence_for<Args...>{});
                break;
            }
            default:
            {
                ostringstream oss;
                (oss << ... << args);
                rawMessage = oss.str();
            }
        }
        
        string timeStamp = getTimeStamp();
        // cout << "lvlstr "<<lvlStr << endl; //testcout
        string levelStr = logLevelToString(level, LOGGER);
        // cout << levelStr << endl; //testcout

        ostringstream logMsg;
        logMsg << timeStamp << levelStr << escapeSpecialChars(rawMessage, LOGGER) << "\n";
        string logStr = logMsg.str();

        if (_logFd != -1 && write(_logFd, logStr.c_str(), logStr.length()) == -1)
        {
            if (_logFd != 3)
                cerr << getTimeStamp() << logLevelToString(ERROR, TERMINAL) << "Writing to log file failed: " << strerror(errno) << endl;
            _logFd = -1;
        }

        if (level >= IWARN || (TERMINAL_DEBUG == true && level == INFO))
        {
            levelStr = logLevelToString(level, TERMINAL);
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
void Logger::logExit(uint8_t level, Args&&... args)
{
    log(level, forward<Args>(args)...);
    throw Logger::ErrorLogExit();
    // exit(EXIT_FAILURE);
}

template<typename... Args>
void Logger::log(uint8_t level, Client &client, Args&&... args)
{
    try
    {
        if (!client._ipPort.first.empty())
        {

            // auto portHostMap = client._usedServer->getPortHost().begin();
            string portHostInfo = client._ipPort.first + ":" + client._ipPort.second;
            // string portHostInfo = "255.255.255.255:65535";
            
            // max size 255.255.255.255:65535
            if (portHostInfo.length() < 21)
                portHostInfo.append(21 - portHostInfo.length(), ' ');
            // log(level, portHostInfo, "  clientFD:", client._fd, "  ", args...);
            log(level, portHostInfo, client._fd, "clientFD", args...);
            return;
        }
        Logger::log(level, client._fd, "clientFD", args...);
        // log(level, client._usedServer->getServerName(), "  FD:", client._fd, "  ", args...);
    }
    catch (...)
    {
        if (_logFd != -1)
            write(_logFd, LOG_ERROR, sizeof(LOG_ERROR) - 1);
        write(STDERR_FILENO, LOG_ERROR, sizeof(LOG_ERROR) - 1);
    }
}
#endif
