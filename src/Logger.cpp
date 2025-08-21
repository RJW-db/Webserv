#include <RunServer.hpp> // escape_special_char   should be utils.hpp
#include "Logger.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <fcntl.h>
// Static member definitions
int Logger::_logFd = -1;

void Logger::initialize(const string &logDir, const string &filename)
{
    string absoluteFilePath;
    if (!logDir.empty())
        absoluteFilePath = initLogDirectory(logDir) + '/' + filename;
    else
        absoluteFilePath = RunServers::getServerRootDir() + '/' + filename;

    _logFd = open(absoluteFilePath.data(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (_logFd == -1)
    {
        Logger::logExit(ERROR, "Log file error", '-', strerror(errno), ' ', absoluteFilePath);
    }

    FileDescriptor::setFD(_logFd);
    log(INFO, "Log file initialized", _logFd, ":logFD", absoluteFilePath);
}

string Logger::initLogDirectory(const string &logDir)
{
    string absolutePath = RunServers::getServerRootDir() + '/' + logDir;
    try
    {
        if (!std::filesystem::exists(absolutePath))
            std::filesystem::create_directory(absolutePath);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        Logger::logExit(ERROR, "Log dir error", '-', "Filesystem error", absolutePath, " Details: ", e.what());
    }
    return absolutePath;
}

string Logger::getTimeStamp()
{
    time_t now = time(0);
    if (now == -1)
        throw runtime_error("std::time failed");

    tm *ltm = localtime(&now);
    
    static int lastDay = -1;
    int currentDay = ltm->tm_mday;
    
    ostringstream timeStamp;
    
    // Print date header if day changed
    if (currentDay != lastDay)
    {
        timeStamp << "\n--- " << ltm->tm_year + 1900 << '-'
                  << setw(2) << setfill('0') << 1 + ltm->tm_mon << '-'
                  << setw(2) << setfill('0') << ltm->tm_mday << " ---\n";
        lastDay = currentDay;
    }
    
    // Just time for each log entry
    timeStamp << '[' << setw(2) << setfill('0') << ltm->tm_hour << ':'
              << setw(2) << setfill('0') << ltm->tm_min << ':'
              << setw(2) << setfill('0') << ltm->tm_sec << "] ";
    
    return timeStamp.str();
}

string Logger::logLevelToString(uint8_t level, bool useColors)
{
    switch (level) {
        case DEBUG: return useColors ? "\033[36mDEBUG\033[0m " : "DEBUG ";
        case INFO:  return useColors ? "\033[32mINFO \033[0m " : "INFO  ";
        case WARN:  return useColors ? "\033[33mWARN \033[0m " : "WARN  ";
        case ERROR: return useColors ? "\033[31mERROR\033[0m " : "ERROR ";
        case FATAL: return useColors ? "\033[91mFATAL\033[0m " : "FATAL ";
        default:    return "????? ";
    }
}
