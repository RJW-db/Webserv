#include <iostream>
#include <FileDescriptor.hpp>
#include <HttpRequest.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>

#include <HandleTransfer.hpp>
#include <ErrorCodeClientException.hpp>
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


HttpRequest::HttpRequest(unique_ptr<Server> &usedServer, Location &loc, int clientFD, ClientRequestState &state)
: _server(usedServer), _location(loc), _clientFD(clientFD), _method(state.method), _headerBlock(state.header), _body(state.body)
{
}

void    validateHEAD(const string &head)
{
    istringstream headStream(head);
    string method, path, version;
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
            // cout << "\tkey\t" << key << "\t" << value << endl;
        }
        start = end + 2;
    }
}

void    HttpRequest::pathHandling()
{
    struct stat status;

    _path = _location.getRoot() + string(_path);
    // std::cout << _path << std::endl;
    if (stat(_path.data(), &status) == -1)
    {
        throw RunServers::ClientException("non existent file");
    }

    if (S_ISDIR(status.st_mode))
    {
        // searching for indexpage in directory
        for (string &indexPage : _location.getIndexPage())
        {
            if (stat(indexPage.c_str(), &status) == 0)
            {
                if (S_ISDIR(status.st_mode) == true ||
                    S_ISREG(status.st_mode) == false)
                {
                    continue;
                }
                if (access(indexPage.data(), R_OK) == -1)
                {
                    cerr << "pathHandling: " << strerror(errno) << endl;
                    continue;
                }
                _path = indexPage; // found index
                std::cout << "\t" << _path << std::endl;
                return;
            }
        }
        // autoindex

        if (_location.getAutoIndex() == true)
        {
            // use cgi using opendir,readdir to create a dynamic html page
        }
        else
        {
            throw ErrorCodeClientException(_clientFD, 404, "couldn't find index page", _location.getErrorCodesWithPage());
        }
    } else if (S_ISREG(status.st_mode))
    {
        if (access(_path.data(), R_OK) == -1)
        {
            cerr << "pathHandling: " << strerror(errno) << endl;
        }
    }
    else
    {
        throw ErrorCodeClientException(_clientFD, 404, "Forbidden: Not a regular file or directory", _location.getErrorCodesWithPage());
    }
}

string HttpRequest::getMimeType(string &path)
{
    static const map<string, string> mimeTypes = {
        { "html", "text/html" },
        { "htm",  "text/html" },
        { "css",  "text/css" },
        { "js",   "application/javascript" },
        { "json", "application/json" },
        { "png",  "image/png" },
        { "jpg",  "image/jpeg" },
        { "jpeg", "image/jpeg" },
        { "gif",  "image/gif" },
        { "svg",  "image/svg+xml" },
        { "ico",  "image/x-icon" },
        { "pdf",  "application/pdf" },
        { "txt",  "text/plain" },
        { "mp4",  "video/mp4" },
        { "webm", "video/webm" }
    };

    size_t dotIndex = path.find_last_of('.');
    if (dotIndex != string::npos)
    {
        string_view extention = string_view(path).substr(dotIndex + 1);
        
        map<string, string>::const_iterator it = mimeTypes.find(extention.data());
        if (it != mimeTypes.end())
            return it->second;
    }
    return "application/octet-stream";
}

void    HttpRequest::GET()
{
    pathHandling();

    int fd = open(_path.data(), R_OK);
    if (fd == -1)
        throw RunServers::ClientException("open failed");

    FileDescriptor::setFD(fd);
    size_t fileSize = getFileLength(_path);
    std::cout << _headerBlock << std::endl;
    string responseStr = HttpResponse(200, _path, fileSize);
    auto handle = make_unique<HandleTransfer>(_clientFD, responseStr, fd, fileSize);
    RunServers::insertHandleTransfer(move(handle));

}


void    HttpRequest::handleRequest(size_t contentLength)
{
	static_cast<void>(contentLength);
    validateHEAD(_headerBlock);

    parseHeaders(_headerBlock);


    
    if (_headers.find("Host") == _headers.end())
        throw RunServers::ClientException("Missing Host header");
    // else if (it->second != "127.0.1.1:8080")
    //     throw Server::ClientException("Invalid Host header: expected 127.0.1.1:8080, got " + string(it->second));
    if (_method == "GET")
    {
        GET();
    }
    else if (_method == "POST")
    {
        // std::cout << _headerBlock << _body << std::endl;
        // Submitting a form (e.g., login, registration, contact form)
        // Uploading a file (e.g., images, documents)
        // Sending JSON data (e.g., for APIs)
        // Creating a new resource (e.g., adding a new item to a database)
        // Triggering an action (e.g., starting a job, sending an email)
        POST();
        // cout << _contentType << '\t' << _bodyBoundary << endl;
    }
    // else
    // {

    // }
    auto it = _headers.find("Connection");
    if (it != _headers.end() && it->second == "keep-alive")
    {
        FileDescriptor::addClientFD(_clientFD);
    }
}


string HttpRequest::HttpResponse(uint16_t code, string path, size_t fileSize)
{
    static const map<uint16_t, string> responseCodes = {
        { 200, "OK" },
        { 400, "Bad Request" },
        { 403, "Forbidden" },
        { 404, "Not Found" },
        { 405, "Method Not Allowed" },
        { 413, "Payload Too Large" },
        { 414, "URI Too Long" },
        { 431, "Request Header Fields Too Large" },
        { 500, "Internal Server Error" },
        { 501, "Not Implemented" },
        { 502, "Bad Gateway" },
        { 503, "Service Unavailable" },
        { 504, "Gateway Timeout" }
    };
        
    map<uint16_t, string>::const_iterator it = responseCodes.find(code);
    if (it == responseCodes.end())
        throw runtime_error("Couldn't find code");

    ostringstream response;
    response << "HTTP/1.1 " << to_string(it->first) << ' ' << it->second << "\r\n";
    response << "Content-Length: " << fileSize << "\r\n";
    response << "Content-Type: " << getMimeType(path) << "\r\n";
    response << "\r\n";

    return response.str();

}