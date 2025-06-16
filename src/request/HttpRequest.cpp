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


HttpRequest::HttpRequest(unique_ptr<Server> &server, int clientFD, string &method, string &header, string &body)
: _server(server), _clientFD(clientFD), _method(method), _headerBlock(header), _body(body)
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
    std::cout << _path << std::endl;
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
            throw ErrorCodeClientException(_clientFD, 403, "couldn't find index page", _location.getErrorCodesWithPage());
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
        throw ErrorCodeClientException(_clientFD, 403, "Forbidden: Not a regular file or directory", _location.getErrorCodesWithPage());
    }
}

string HttpRequest::getMimeType(void)
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

    size_t dotIndex = _path.find_last_of('.');
    if (dotIndex != string::npos)
    {
        string_view extention = string_view(_path).substr(dotIndex + 1);
        
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
    RunServers::setEpollEvents(_clientFD, EPOLL_CTL_MOD, EPOLLIN | EPOLLOUT);
    size_t fileSize = getFileLength(_path);

    std::cout << getMimeType() << std::endl;
    
    ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Length: " << fileSize << "\r\n";
    // response << "Content-Type: text/html\r\n";
    response << "Content-Type: " << getMimeType() << "\r\n";
    response << "\r\n";

    string responseStr = response.str();
    auto handle = make_unique<HandleTransfer>(_clientFD, responseStr, fd, fileSize);
    RunServers::insertHandleTransfer(move(handle));

}

void HttpRequest::setLocation()
{
	size_t pos = string_view(_headerBlock).find_first_not_of(" \t", _method.length());
	if (pos == string::npos || _headerBlock[pos] != '/')
		throw RunServers::ClientException("missing path in HEAD");
	size_t len = string_view(_headerBlock).substr(pos).find_first_of(" \t\n\r");
	_path = string_view(_headerBlock).substr(pos, len);
	for (pair<string, Location> &locationPair : _server->getLocations())
	{
		if (strncmp(_path.data(), locationPair.first.c_str(), locationPair.first.length()) == 0)
		{
			_location = locationPair.second;
			return ;
		}
	}
	// throw RunServers::ClientException("No matching location found for path: " + string(_path));
}

void    HttpRequest::handleRequest(size_t contentLength)
{
	static_cast<void>(contentLength);
	setLocation();
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
        std::cout << _headerBlock << _body << std::endl;
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
}
