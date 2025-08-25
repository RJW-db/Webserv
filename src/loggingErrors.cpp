#include <RunServer.hpp>
#include <sstream>  // For std::ostringstream
#include <iomanip> // For std::setw and std::setfill

string timeDate(void)
{
    time_t now = time(0);
    tm *ltm = localtime(&now);
    std::ostringstream _timeDate;

    _timeDate << '[' << ltm->tm_year + 1900 << '-';                           // Year
    _timeDate << std::setw(2) << std::setfill('0') << 1 + ltm->tm_mon << '-'; // Month
    _timeDate << std::setw(2) << std::setfill('0') << ltm->tm_mday << ' ';    // Day
    _timeDate << std::setw(2) << std::setfill('0') << ltm->tm_hour << ':';    // Hour
    _timeDate << std::setw(2) << std::setfill('0') << ltm->tm_min << ':';     // Minute
    _timeDate << std::setw(2) << std::setfill('0') << ltm->tm_sec << "] ";    // Second

    return _timeDate.str(); // Convert the stream to a string and return it
}

std::string escapeSpecialChars(const std::string& input)
{
    const std::string RED = "\033[31m";
    const std::string MAGENTA = "\033[35m";
    const std::string RESET = "\033[0m";

    std::string result;
    for (char ch : input) {
        if (ch == '\r') {
            result += RED + "\\r" + RESET;
        } else if (ch == '\n') {
            result += MAGENTA + "\\n\n" + RESET;  // Literal \n + actual newline
        } else if (ch == '\t') {
            result += RED + "\\t" + RESET;
        } else {
            result += ch;
        }
    }
	return result;
}
