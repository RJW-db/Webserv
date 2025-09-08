#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <fstream>
#include <string>
#include "Client.hpp"
#include "utils.hpp"
#ifndef TERMINAL_DEBUG
# define TERMINAL_DEBUG true
#endif
using namespace std;
namespace
{
    constexpr char LOG_ERROR[] = "Logger error occurred\n";
    
    enum LogLevel : uint8_t
    {
        CHILD_INFO = 0, // cout
        INFO       = 1, // cout
        IWARN      = 2, // cerr
        WARN       = 3, // cerr
        IERROR     = 4, // cerr
        ERROR      = 5, // cerr
        FATAL      = 6, // cerr
        DEBUG      = 7  // cerr
    };
}

class Logger
{
    public:
        static void initialize(const string &logDir, const string &logFilename);
        static string initLogDirectory(const string &logDir);

        template<typename... Args>
        static void log(uint8_t level, Args&&... args);

        template<typename... Args>
        static void logExit(uint8_t level, Args&&... args);

        template<typename... Args>
        static void log(uint8_t level, Client &client, Args&&... args);

        template<typename... Args>
        static string makeRawMessage(uint8_t level, Args&&... args);

        static inline int getLogfd() { return _logFd; }

        class ErrorLogExit : public exception
        {
        public:
            explicit ErrorLogExit() noexcept {}
            const char *what() const noexcept override {
                return "Exiting";
            }
        };

    private:
        static string getTimeStamp();
        static string logLevelToString(uint8_t level, bool useColors);

        static inline string padRight(const string& s, size_t width) {
            return (s.size() < width) ? s + string(width - s.size(), ' ') : s;
        }

        // Left pad (for numbers, right-align)
        static inline string padLeft(const string& s, size_t width) {
            return (s.size() < width) ? string(width - s.size(), ' ') + s : s;
        }

        template<typename T>
        static string argToString(const T& arg);

        template<typename Tuple, size_t... Is>
        static string processArgsToString(const Tuple& tup, index_sequence<Is...>);

        template<typename Tuple, size_t... Is>
        static string processErrorArgsToString(const Tuple& tup, index_sequence<Is...>);

        static int _logFd;
};

template<typename... Args>
void Logger::log(uint8_t level, Args&&... args)
{
    try
    {
        string rawMessage = makeRawMessage(level, forward<Args>(args)...);
        
        string timeStamp = getTimeStamp();
        string levelStr = logLevelToString(level, LOGGER);
        ostringstream logMsg;
        logMsg << timeStamp << levelStr << escapeSpecialChars(rawMessage, LOGGER) << "\n";
        
        string logStr = logMsg.str();

        if (_logFd != -1 && write(_logFd, logStr.c_str(), logStr.length()) == -1)
        {
            cerr << getTimeStamp() << logLevelToString(ERROR, TERMINAL) << "Writing to log file failed: " << strerror(errno) << endl;
            _logFd = -1;
        }

        if (level >= IWARN || (TERMINAL_DEBUG == true && level == INFO))
        {
            levelStr = logLevelToString(level, TERMINAL);
            ostream& output = (level == INFO) ? cout : cerr;
            output << timeStamp + levelStr + escapeSpecialChars(rawMessage, TERMINAL) + "\n";
            // output << timeStamp + levelStr + escapeSpecialChars(rawMessage, TERMINAL) + "     errno=" + to_string(errno) + "\n";
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
}

template<typename... Args>
void Logger::log(uint8_t level, Client &client, Args&&... args)
{
    try
    {
        if (!client._ipPort.first.empty())
        {
            string portHostInfo = client._ipPort.first + ":" + client._ipPort.second;

            if (portHostInfo.length() < 21)
                portHostInfo.append(21 - portHostInfo.length(), ' ');
            log(level, portHostInfo, client._fd, "clientFD", args...);
            return;
        }
        Logger::log(level, client._fd, "clientFD", args...);
    }
    catch (...)
    {
        if (_logFd != -1)
            write(_logFd, LOG_ERROR, sizeof(LOG_ERROR) - 1);
        write(STDERR_FILENO, LOG_ERROR, sizeof(LOG_ERROR) - 1);
    }
}

template<typename... Args>
string Logger::makeRawMessage(uint8_t level, Args&&... args)
{
    switch (level)
    {
        case CHILD_INFO:
        case INFO:
        case IWARN:
        case IERROR:
        {
            auto tup = forward_as_tuple(forward<Args>(args)...);
            return processArgsToString(tup, index_sequence_for<Args...>{});
        }
        case WARN:
        case ERROR:
        case FATAL:
        {
            auto tup = forward_as_tuple(forward<Args>(args)...);
            return processErrorArgsToString(tup, index_sequence_for<Args...>{});
        }
        default:
        {
            ostringstream oss;
            (oss << ... << args);
            return oss.str();
        }
    }
}

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
        Is == 0 ? Logger::padRight(Logger::argToString(get<Is>(tup)), 24) :       // left-aligned message
        Is == 1 ? Logger::padLeft(Logger::argToString(get<Is>(tup)), 6) :         // right-aligned number/dash
        Is == 2 ? ':' + Logger::padRight(Logger::argToString(get<Is>(tup)), 14) : // left-aligned label/reason
        Logger::argToString(get<Is>(tup))                                         // rest (no padding)
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
        Is == 2 ? string(15, ' ') + Logger::argToString(get<Is>(tup)) :     // always 15 spaces before reason
        Logger::argToString(get<Is>(tup))                                   // rest (no padding)
    )), ...);
    return out;
}
#endif
