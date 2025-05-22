#include <Webserv.hpp>
#include <iostream>
#include <FileDescriptor.hpp>

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // close
#include <stdlib.h>	// callod
#ifdef __linux__
#include <sys/epoll.h>
#endif
#include <sys/socket.h>

#include <dirent.h>

#include <signal.h>

#include <fstream>
#include <sstream>
#include <sys/stat.h>


void ValidateHEAD(const string &head)
{
    std::istringstream headStream(head);
    std::string method, path, version, check;
    headStream >> method >> path >> version;
    if (/* method != "HEAD" &&  */method != "GET" && method != "POST" && method != "DELETE")
    {
        throw Server::ClientException("Invalid HTTP method: " + method);
    }

    if (path.empty() || path.c_str()[0] != '/')
    {
        throw Server::ClientException("Invalid HTTP path: " + path);
    }

    if (version != "HTTP/1.1")
    {
        throw Server::ClientException("Invalid HTTP version: " + version);
    }

    cout << method << endl;
    cout << path << endl; // find "?" to if it has a query
    cout << version << endl;
    cout << check << endl;
    // throw runtime_error("CLOSING RIGHT HERE RIGHT NOW");
}

void validateLines(const std::string &line)
{
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos || colonPos == 0 || colonPos == line.size() - 1)
    {
        throw Server::ClientException("Invalid HTTP request, malformed line: " + line);
    }

    std::string key = line.substr(0, colonPos);
    std::string value = line.substr(colonPos + 1);

    // Trim whitespace from key and value
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);

    if (key.empty() || value.empty())
    {
        throw Server::ClientException("Invalid HTTP request, malformed line: " + line);
        // throw std::runtime_error("Invalid HTTP request: Empty line key or value");
    }
}

void parseHttpRequest(string &request)
{
	// ofstream myfile;
	// myfile.open ("clientHttpRequest.txt");
	// myfile << request.c_str();
	// myfile.close();

    // escape_special_chars(request);

    const std::string terminator = "\r\n";
    size_t start = 0;
    size_t end = request.find(terminator, start);

    // Validate the request line (first line)
    if (end == std::string::npos)
    {
        // throw Server::ClientException("Invalid HTTP request, malformed line: " + line);
        // throw std::runtime_error("Invalid HTTP request: Missing request line terminator");
    }
    std::string head = request.substr(start, end - start);
    ValidateHEAD(head);

    start = end + terminator.length();
    end = request.find(terminator, start);

    while (end != string::npos) {
        string line = request.substr(start, end - start);

        // Validate the line (e.g., ensure it's not empty for headers)
        if (line.empty() && start != 0) {
            // Blank line found, stop checking further (end of headers)
            break;
        }
        validateLines(line);
        start = end + 2; // Move past \r\n
        end = request.find("\r\n", start); // Find the next \r\n
    }
    // Ensure there is a blank line after headers
    if (end == string::npos || start == request.size())
    {
        // string errorLine = request.substr(0, request.size() - start); // Capture the problematic line
        httpRequestLogger(request);
        // throw Server::ClientException("Invalid HTTP request, malformed line: " + line);
        // throw runtime_error("HTTP/1.1 400 Bad Request");// HTTP/1.1 400 Bad Request, <html><body><h1>400 Bad Request</h1></body></html>
    }
}
