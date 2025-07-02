#include <RunServer.hpp>
#include <sstream>  // For std::ostringstream
#include <iomanip> // For std::setw and std::setfill

// [Wed May 21 13:15:01.234567 2025] [core:error] [pid 1234] [client 192.168.1.100:56789] AH00526: Syntax error in request: GET /favicon.ico HTTP/1.1\r\n


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

// if (!is_valid_http_request(request)) {
//     std::ofstream log("error.log", std::ios::app);
//     log << "[ERROR] Invalid HTTP request received:\n"
//         << escape_special_chars(request) << "\n\n";
// }
void httpRequestLogger(string str)
{
    string result = timeDate();

    result += "[client <ip>:<port>] " + escape_special_chars(str);
    // std::cout << "\t>" << str << "<" << std::endl;
    std::cout << "httprequestlogerr: " << result << std::endl;
}


std::string escape_special_chars(const std::string& input)
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
