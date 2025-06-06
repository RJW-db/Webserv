#include <iostream>
#include <FileDescriptor.hpp>
#include <HttpRequest.hpp>
#include <RunServer.hpp>

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


HttpRequest::HttpRequest(unique_ptr<Server> &server, int clientFD, string &method, string &header, string &body)
: _server(server), _clientFD(clientFD), _method(method), _headerBlock(header), _body(body)
{
}

void    validateHEAD(const string &head)
{
    std::istringstream headStream(head);
    std::string method, path, version;
    headStream >> method >> path >> version;
    
    if (/* method != "HEAD" &&  */method != "GET" && method != "POST" && method != "DELETE")
    {
        throw RunServers::ClientException("Invalid HTTP method: " + method);
    }
   
    if (path.empty() || path.c_str()[0] != '/')
    {
        throw RunServers::ClientException("Invalid HTTP path: " + path);
    }
    // if (path == uploadStore)

    if (version != "HTTP/1.1")
    {
        throw RunServers::ClientException("Invalid HTTP version: " + version);
    }
}


void HttpRequest::parseHeaders(const string& headerBlock)
{
    size_t start = 0;
    while (start < headerBlock.size())
    {
        size_t end = headerBlock.find("\r\n", start);
        if (end == string::npos)
            throw RunServers::ClientException("Malformed HTTP request: header line not properly terminated");

        string_view line(&headerBlock[start], end - start);
        if (line.empty())
            break; // End of headers

        size_t colon = line.find(':');
        if (colon != string_view::npos) {
            // Extract key and value as string_views
            string_view key = line.substr(0, colon);
            string_view value = line.substr(colon + 1);

            // Trim whitespace from key
            key.remove_prefix(key.find_first_not_of(" \t"));
            key.remove_suffix(key.size() - key.find_last_not_of(" \t") - 1);
            // Trim whitespace from value
            value.remove_prefix(value.find_first_not_of(" \t"));
            value.remove_suffix(value.size() - value.find_last_not_of(" \t") - 1);

            _headers[string(key)] = value;
            // std::cout << "\tkey\t" << key << "\t" << value << std::endl;
        }
        start = end + 2;
    }
}



void    HttpRequest::GET()
{
    std::ifstream file("webPages/POST_upload.html", std::ios::in | std::ios::binary);
    if (!file)
    {
        std::string notFound = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(_clientFD, notFound.c_str(), notFound.size(), 0);
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
    send(_clientFD, responseStr.c_str(), responseStr.size(), 0);
}



void    HttpRequest::handleRequest(size_t contentLength)
{
	static_cast<void>(contentLength);

    // std::cout << escape_special_chars(_headerBlock) << std::endl;
    validateHEAD(_headerBlock);

    parseHeaders(_headerBlock);

    if (_headers.find("Host") == _headers.end())
        throw RunServers::ClientException("Missing Host header");
    // else if (it->second != "127.0.1.1:8080")
    //     throw Server::ClientException("Invalid Host header: expected 127.0.1.1:8080, got " + std::string(it->second));

    if (_method == "GET")
    {
        GET();
    }
    else if (_method == "POST")
    {

        // Submitting a form (e.g., login, registration, contact form)
        // Uploading a file (e.g., images, documents)
        // Sending JSON data (e.g., for APIs)
        // Creating a new resource (e.g., adding a new item to a database)
        // Triggering an action (e.g., starting a job, sending an email)
        POST();
        // std::cout << _contentType << '\t' << _bodyBoundary << std::endl;
    }
    // else
    // {

    // }
}
