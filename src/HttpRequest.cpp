#include <Webserv.hpp>
#include <iostream>
#include <FileDescriptor.hpp>

#include <unordered_set>
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


string ValidateHEAD(const string &head)
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

    // cout << method << endl;
    // cout << path << endl; // find "?" to if it has a query
    // cout << version << endl;
    // cout << check << endl;

    // throw runtime_error("CLOSING RIGHT HERE RIGHT NOW");
    return method;
}

void    headerNameContentType(string &type)
{
    if (type != "application/x-www-form-urlencoded" && // Standard HTML form data
        type != "application/json" &&
        type != "text/plain" &&
        strncmp("multipart/form-data; boundary=", type.c_str(), 30) != 0)
    {
        throw Server::ClientException("Invalid HTTP request, we don't handle this Content-Type: " + type);
    }
}
#include <algorithm>
#include <string>
void    headerNameContentLength(string &length)
{
    try
    {
        long long value = stoll(length); // Attempt to convert to a number
        if (value < 0) // Ensure the value is non-negative
        {
            throw Server::ClientException("Invalid HTTP request, Content-Length cannot be negative: " + length);
        }
        if (true/* value > client_max_body_size */)
        {
            throw Server::ClientException("Invalid HTTP request, Content-Length is out of range: " + length);
        }
    }
    catch (const std::invalid_argument &)
    {
        throw Server::ClientException("Invalid HTTP request, Content-Length contains non-digit characters: " + length);
    }
    catch (const std::out_of_range &)
    {
        throw Server::ClientException("Invalid HTTP request, Content-Length is out of range: " + length);
    }
    // if content-type is multipart/form-data, lenght should be bigger then 0
    // check if positive and only digits

    // if (client_max_body_size)
    (void)length;
}

void    headerNameHost(string &host)
{
    size_t colonPos = host.find(':');   // e.g. en.wikipedia.org:8080, en.wikipedia.org
    if (colonPos == string::npos || colonPos == 0 || colonPos == host.size() - 1)
    {
        throw Server::ClientException("Invalid HTTP request, malformed header field: " + host);
    }
    string value = host.substr(colonPos + 1);
    if (value != "8080") // TODO 8080 is now hardcoded
    {
        throw Server::ClientException("Invalid HTTP request, wrong port: " + value);
    }
}

void validateLines(const std::string &line, unordered_set<string> &requiredHeaders)
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

    // 1. must handle for POST
    // Host Content-Length Content-Type

    // 2. nice to have extra
    // Connection User-Agent Accept Referer Cookie Authorization Expect

    // headerName      Host
    // headerValue     example.com
    static const std::unordered_map<std::string, void(*)(std::string&)> headerHandlers = {
        {"Host", headerNameHost},
        {"Content-length", headerNameContentLength},
        {"Content-Type", headerNameContentType}
    };

    unordered_map<string, void(*)(string&)>::const_iterator it = headerHandlers.find(key);
    if (it != headerHandlers.end())
    {
        it->second(value);
    }

    if (requiredHeaders.find(key) != requiredHeaders.end())
    {
        requiredHeaders.erase(key); // Mark the header as found
    }
}

void saveRequestToFile(const std::string &request) {
    std::string modifiableRequest = request; // Create a copy
    size_t pos = 0;
    while ((pos = modifiableRequest.find("\r\r", pos)) != std::string::npos) {
        modifiableRequest.erase(pos, 1); // Remove the extra \r
    }

    std::ofstream myfile("clientHttpRequest.txt", std::ios::out | std::ios::binary);
    if (!myfile) {
        std::cerr << "Failed to open file for writing." << std::endl;
        return;
    }
    myfile.write(modifiableRequest.c_str(), modifiableRequest.size());
    myfile.close();
}

void parseHttpRequest(string &request)
{
    // saveRequestToFile(request);
    cout << escape_special_chars(request) << endl;

    const std::string terminator = "\r\n";
    size_t start = 0;
    size_t end = request.find(terminator, start);

    // Validate the request line (first line)
    if (end == std::string::npos)
    {
        throw Server::ClientException("Invalid HTTP request, missing: \"\\r\\n\"");
    }
    std::string head = request.substr(start, end - start);
    string method = ValidateHEAD(head);
    unordered_set<string> requiredHeaders;
    if (method == "POST")
        requiredHeaders = {"Host", "Content-Length", "Content-Type"};
    start = end + terminator.length();
    end = request.find(terminator, start);
    while (end != string::npos) {
        string line = request.substr(start, end - start);

        if (line.empty()) {
            if (start != 0) {
                // Blank line found, stop checking further (end of headers)
                break;
            }
            // Otherwise, continue to the next line
            start = end + 2; // Move past \r\n
            end = request.find("\r\n", start);
            continue;
        }
        //  -----TEMPORARY for testing with clientHttpRequst.txt-----
        if (line.size() == 2 && line[0] == '\r') {
            break;
        }
        //  -----TEMPORARY-----

        std::cout << ">" << escape_special_chars(line) << "<" << std::endl;
        validateLines(line, requiredHeaders);

        start = end + 2; // Move past \r\n
        end = request.find("\r\n", start); // Find the next \r\n
    }
    // Ensure there is a blank line after headers
    if (end == string::npos || start == request.size())
    {
        // string errorLine = request.substr(0, request.size() - start); // Capture the problematic line
        // httpRequestLogger(request);
        // sendErrorResponse(clientFD, "411 Length Required");

        // throw Server::ClientException("Invalid HTTP request, malformed line: " + line);
        // throw runtime_error("HTTP/1.1 400 Bad Request");// HTTP/1.1 400 Bad Request, <html><body><h1>400 Bad Request</h1></body></html>
    }
    if (!requiredHeaders.empty())
    {
        throw Server::ClientException("Missing required headers in HTTP request");
    }
    else
        std::cout << "connection was established" << std::endl;
}


void    validateHEAD(const string &head)
{
    std::istringstream headStream(head);
    std::string method, path, version;
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
}
void    validateKeyValues(string request, const string &method)
{
    const std::string terminator = "\r\n";
    size_t start = 0;
    size_t end = request.find(terminator, start);
    // Validate the request line (first line)
    if (end == std::string::npos)
    {
        throw Server::ClientException("Invalid HTTP request, missing: \"\\r\\n\"");
    }
    unordered_set<string> requiredHeaders ;
    if (method == "GET")
        requiredHeaders = {"Host"};
    else if (method == "POST")
        requiredHeaders = {"Host", "Content-Length", "Content-Type"};
    start = end + terminator.length();
    end = request.find(terminator, start);

    while (end != string::npos)
    {
        string line = request.substr(start, end - start);

        if (line.empty()) {
            if (start != 0) {
                // Blank line found, stop checking further (end of headers)
                break;
            }
            // Otherwise, continue to the next line
            start = end + 2; // Move past \r\n
            end = request.find("\r\n", start);
            continue;
        }
        //  -----TEMPORARY for testing with clientHttpRequst.txt-----
        if (line.size() == 2 && line[0] == '\r') {
            break;
        }
        //  -----TEMPORARY-----

        validateLines(line, requiredHeaders);

        start = end + 2; // Move past \r\n
        end = request.find("\r\n", start); // Find the next \r\n
        string tmp = request.substr(start, end - start);
    }
    // Ensure there is a blank line after headers
    if (end == string::npos || start == request.size())
    {
        std::cerr << "Debug Info: end=" << end << ", start=" << start << ", request.size()=" << request.size() << std::endl;
        throw Server::ClientException("Invalid HTTP request, malformed or incomplete headers.");
        // string errorLine = request.substr(0, request.size() - start); // Capture the problematic line
        // httpRequestLogger(request);
        // throw Server::ClientException("Invalid HTTP request, malformed line: " + line);
        // throw runtime_error("HTTP/1.1 400 Bad Request");// HTTP/1.1 400 Bad Request, <html><body><h1>400 Bad Request</h1></body></html>
    }
    if (!requiredHeaders.empty())
    {
        throw Server::ClientException("Missing required headers in HTTP request");
    }
    else
        std::cout << "connection was established" << std::endl;
}
void    handleRequest(int clientFD, string &method, string &header, string &body)
{
    std::cout << "vor" << std::endl;
    std::cout << method << std::endl;
    validateHEAD(header);
    validateKeyValues(header, method);
    if (method == "GET")
    {
        // std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        // send(clientFD, response.c_str(), response.size(), 0);

        std::ifstream file("webPages/POST_upload.html", std::ios::in | std::ios::binary);
        if (!file)
        {
            std::string notFound = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            send(clientFD, notFound.c_str(), notFound.size(), 0);
            return;
        }

        // Read file contents
        std::ostringstream ss;
        ss << file.rdbuf();
        std::string fileContent = ss.str();

        // Build response
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Length: " << fileContent.size() << "\r\n";
        response << "Content-Type: text/html\r\n";
        response << "\r\n";
        response << fileContent;

        std::string responseStr = response.str();
        send(clientFD, responseStr.c_str(), responseStr.size(), 0);
    }
    else if (method == "POST")
    {
        std::cout << "hier zijn we\n\n\n" << std::endl;
        // Submitting a form (e.g., login, registration, contact form)
        // Uploading a file (e.g., images, documents)
        // Sending JSON data (e.g., for APIs)
        // Creating a new resource (e.g., adding a new item to a database)
        // Triggering an action (e.g., starting a job, sending an email)
        std::cout << header << std::endl;
        std::cout << escape_special_chars(body) << std::endl;

        const string boundaryKey = "Content-Type: multipart/form-data; boundary=";
        size_t position = header.find(boundaryKey);

        size_t boundaryStart = position + boundaryKey.length();
        size_t boundaryEnd = header.find("\r\n", boundaryStart);

        std::string bodyBoundary = header.substr(boundaryStart, boundaryEnd - boundaryStart);
        std::cout << "bodyBoundary " << bodyBoundary << std::endl;

        
        const string bodyKey = "Content-Type: ";
        size_t fileStart = body.find(bodyKey);
        fileStart = body.find("\r\n\r\n", fileStart) + 4;
        size_t fileEnd = body.find("\r\n--" + bodyBoundary, fileStart);

        std::string file = body.substr(fileStart, fileEnd - fileStart);
        std::cout << "file >" << escape_special_chars(file) << "<" << std::endl;
        // if (file == string::npos)
        // {
        //     std::cout << "not ogod" << std::endl;
        // }
        ofstream myfile;
        // myfile.open("example.txt");
        // myfile << file;
        // myfile.close();
        myfile.open("ssam.png");
        myfile << file;
        myfile.close();
    }
    // else
    // {

    // }
    std::cout << "\t" << method << std::endl;
}