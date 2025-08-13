#include <RunServer.hpp> // escape_special_char   should be utils.hpp
#include "Logger.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

// Static member definitions
ofstream Logger::_logFile;

void Logger::initialize(const string& logPath)
{
    try {
        // Create directory if it doesn't exist
        filesystem::path logDir = filesystem::path(logPath).parent_path();
        if (!logDir.empty()) {
            filesystem::create_directories(logDir);
        }
        
        // Open file in append mode (creates if doesn't exist, appends if exists)
        _logFile.open(logPath, ios::app);
        if (!_logFile.is_open()) {
            throw runtime_error("Failed to open log file: " + logPath);
        }
        
        // Log initialization success
        log(INFO, "Log file initialized: ", logPath);
        
    } catch (const exception& e) {
        cerr << "FATAL: Cannot initialize logger: " << e.what() << endl;
        cerr << "Webserver cannot continue without logging capability." << endl;
        exit(1); // Stop the webserver
    }
}

void Logger::cleanup()
{
    if (_logFile.is_open()) {
        log(0, "[LOGGER] Shutting down logger");
        _logFile.close();
    }
}

string Logger::getTimeStamp()
{
    time_t now = time(0);
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

string Logger::logLevelToString(uint8_t level)
{
    switch (level) {
        case DEBUG: return "DEBUG ";
        case INFO:  return "INFO  ";
        case WARN:  return "WARN  ";
        case ERROR: return "ERROR ";
        case FATAL: return "FATAL ";
        default:    return "????? ";
    }
}
