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

    static inline string padInfoField(const string& info, size_t width)
    {
        return (info.size() < width) ? info + string(width - info.size(), ' ') : info;
    }
    static inline string padNumField(const string& num, size_t width)
    {
        return (num.size() < width) ? string(width - num.size(), ' ') + num : num;
    }

    template<typename T>
    static string argToString(const T& arg);

    template<typename Tuple, std::size_t... Is>
    static string processArgsToString(const Tuple& tup, std::index_sequence<Is...>);

public:
    static void initialize(const string &logDir, const string &logFilename);
    static string initLogDirectory(const string &logDir);


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

// Helper to convert any type to string
template<typename T>
string Logger::argToString(const T& arg)
{
    std::ostringstream oss;
    oss << arg;
    return oss.str();
}

// Helper to process each argument by index
template<typename Tuple, std::size_t... Is>
std::string Logger::processArgsToString(const Tuple& tup, std::index_sequence<Is...>)
{
    std::string out;
    ((out += (
        Is == 0 ? Logger::padInfoField(Logger::argToString(std::get<Is>(tup)), 24) : // message
        Is == 1 ? Logger::padNumField(Logger::argToString(std::get<Is>(tup)), 6) :   // number (right-aligned)
        Is == 2 ? Logger::padInfoField(Logger::argToString(std::get<Is>(tup)), 14) : // label (left-aligned, 14 chars)
        Logger::argToString(std::get<Is>(tup))                                       // rest (no padding)
    )), ...);
    return out;
}

template<typename... Args>
void Logger::log(int level, Args&&... args)
{
    (void)level;
    try
    {
        // auto tup = std::forward_as_tuple(std::forward<Args>(args)...);
        // processArgs(tup, std::index_sequence_for<Args...>{});
        // exit(0);

        auto tup = std::forward_as_tuple(std::forward<Args>(args)...);
        std::string rawMessage = processArgsToString(tup, std::index_sequence_for<Args...>{});

        std::string timeStamp = getTimeStamp();
        int lvlStr = (level == CHILD_INFO) ? level + 1 : level;
        std::string levelStr = logLevelToString(lvlStr, LOGGER);

        std::ostringstream logMsg;
        logMsg << timeStamp << levelStr << escapeSpecialChars(rawMessage, LOGGER) << "\n";
        std::string logStr = logMsg.str();

        if (_logFd != -1 && write(_logFd, logStr.c_str(), logStr.length()) == -1)
        {
            if (_logFd != 3)
                cerr << getTimeStamp() << logLevelToString(ERROR, TERMINAL) << "Writing to log file failed: " << strerror(errno) << endl;
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
        if (!client._ipPort.first.empty())
        {

            // auto portHostMap = client._usedServer->getPortHost().begin();
            string portHostInfo = client._ipPort.first + ":" + client._ipPort.second;
            // string portHostInfo = "255.255.255.255:65535";
            
            // max size 255.255.255.255:65535
            if (portHostInfo.length() < 21)
                portHostInfo.append(21 - portHostInfo.length(), ' ');
            // log(level, portHostInfo, "  clientFD:", client._fd, "  ", args...);
            log(level, portHostInfo, client._fd, ":clientFD", args...);
            return;
        }
        Logger::log(level, client._fd, ":clientFD", args...);
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
