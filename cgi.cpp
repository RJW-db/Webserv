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

int main() {
    // Print HTTP header
    std::cout << "Content-Type: text/html\r\n\r\n";

    // HTML start
    std::cout << "<html><head><title>CGI Output</title></head><body>\n";
    std::cout << "<h1>C++ CGI Dynamic Page</h1>\n";

    std::string method = getenv("REQUEST_METHOD") ? getenv("REQUEST_METHOD") : "";
    std::map<std::string, std::string> params;

    if (method == "GET") {
        std::string query = getenv("QUERY_STRING") ? getenv("QUERY_STRING") : "";
        params = parseQueryString(query);
    }
    else if (method == "POST") {
        int content_length = std::atoi(getenv("CONTENT_LENGTH"));
        std::string body;
        body.resize(content_length);
        std::cin.read(&body[0], content_length);
        params = parseQueryString(body);
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
    std::cout << "</ul>";

    // HTML end
    std::cout << "</body></html>\n";

    std::cout << "\nwe were hererererere" << std::endl; //testcout
    return 0;
}
