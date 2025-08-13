#include <iostream>
#include <cstdlib>
#include <string>
#include <map>
#include <sstream>

std::map<std::string, std::string> parseQueryString(const std::string& query) {
    std::map<std::string, std::string> result;
    std::istringstream stream(query);
    std::string pair;

    while (std::getline(stream, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            result[key] = value;
        }
    }
    return result;
}
#include <fcntl.h>
#include <unistd.h>
int main() {
    // Print HTTP header
    std::cout << "Content-Type: text/html\r\n\r\n";

    // HTML start
    std::cout << "<html><head><title>CGI Output</title></head><body>\n";
    std::cout << "<h1>C++ CGI Dynamic Page</h1>\n";

    std::string method = getenv("REQUEST_METHOD") ? getenv("REQUEST_METHOD") : "";
    std::cout << "method " << method << std::endl; //testcout
    std::map<std::string, std::string> params;

    if (method == "GET") {
        std::string query = getenv("QUERY_STRING") ? getenv("QUERY_STRING") : "";
        params = parseQueryString(query);
    }
    else if (method == "POST") {
        sleep(1);
        int content_length = std::atoi(getenv("CONTENT_LENGTH"));
        std::string body;
        body.resize(content_length);
        char buff[content_length];
        int readd = read(0, buff, 4);
        // std::cin.read(&body[0], content_length);
        write(1, buff, readd);
        write(1, "\n", 1);
        // params = parseQueryString(buff);
        // std::cout << "content length: " << content_length << std::endl; //testcout
    }

    if (params.empty()) {
        std::cout << "<p>No input received.</p>\n";
    } else {
        std::cout << "<h2>Received Parameters:</h2><ul>\n";
        for (std::map<std::string, std::string>::iterator it = params.begin(); it != params.end(); ++it) {
            std::cout << "<li><strong>" << it->first << "</strong>: " << it->second << "</li>\n";
        }
        std::cout << "</ul>\n";
    }

    // Debug info (optional)
    std::cout << "<h3>Environment Info:</h3><ul>";
    std::cout << "<li>REQUEST_METHOD: " << method << "</li>";
    std::cout << "<li>QUERY_STRING: " << (getenv("QUERY_STRING") ? getenv("QUERY_STRING") : "") << "</li>";
    std::cout << "</ul>\n";

    // HTML end
    std::cout << "</body></html>\n";
    return 0;
}
