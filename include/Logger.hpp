#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

enum LogLevel : uint8_t {
    DEBUG = 0,
    INFO  = 1,
    WARN  = 2,
    ERROR = 3,
    FATAL = 4
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
    static void log(int level, Args&&... args);  // Don't forget 'static'!
};

// Template implementation outside the class
template<typename... Args>
void Logger::log(int level, Args&&... args)
{
  
    ostringstream oss;
    (oss << ... << args);
    string rawMessage = oss.str();
    
    string timeStamp = getTimeStamp();
    string levelStr = logLevelToString(level);
    string escapedMessage = escapeSpecialCharsRaw(rawMessage);
    string fullMessage = timeStamp + levelStr + escapedMessage;
    
    // Always write to console
    cout << levelStr + escapeSpecialChars(rawMessage) << endl;
    
    // Write to file if available
    if (_logFile.is_open()) {
        _logFile << fullMessage << endl;
        _logFile.flush();
    }
}

#endif