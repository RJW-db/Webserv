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
# include <sys/epoll.h>
#endif
#include <sys/socket.h>

#include <dirent.h>

#include <signal.h>

#include <fstream>
#include <sstream>
#include <sys/stat.h>

void    HttpRequest::validateHEAD(Client &client)
{
    istringstream headStream(client._header);
    headStream >> client._method >> client._path >> client._version;
    
    if (/* client._method != "HEAD" &&  */client._method != "GET" && client._method != "POST" && client._method != "DELETE")
    {
        throw RunServers::ClientException("Invalid HTTP method: " + client._method);
        // sendErrorResponse(client._fd, "400 Bad Request");
    }

    if (client._path.empty() || client._path.data()[0] != '/')
    {
        throw RunServers::ClientException("Invalid HTTP path: " + client._path);
    }

    if (client._version != "HTTP/1.1")
    {
        throw RunServers::ClientException("Invalid HTTP version: " + client._version);
    }
}


void HttpRequest::parseHeaders(Client &client)
{
    size_t start = 0;
    while (start < client._header.size())
    {
        size_t end = client._header.find("\r\n", start);
        if (end == string::npos)
            throw RunServers::ClientException("Malformed HTTP request: header line not properly terminated");

        string_view line(&client._header[start], end - start);
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

            client._headerFields[string(key)] = value;
            // cout << "\tkey\t" << key << "\t" << value << endl;
        }
        start = end + 2;
    }
}

void    HttpRequest::locateRequestedFile(Client &client)
{
    struct stat status;
    client._path = client._location.getRoot() + string(client._path);
    // cout << client._path << endl;
    if (stat(client._path.data(), &status) == -1)
    {
        throw RunServers::ClientException("non existent file");
    }

    if (S_ISDIR(status.st_mode))
    {
        // searching for indexpage in directory
        for (string &indexPage : client._location.getIndexPage())
        {
            // cout << "indexPage " << indexPage << endl;
            if (stat(indexPage.data(), &status) == 0)
            {
                if (S_ISDIR(status.st_mode) == true ||
                    S_ISREG(status.st_mode) == false)
                {
                    continue;
                }
                if (access(indexPage.data(), R_OK) == -1)
                {
                    cerr << "locateRequestedFile: " << strerror(errno) << endl;
                    continue;
                }
                client._path = indexPage; // found index
                // cout << "\t" << client._path << endl;
                return;
            }
        }
        // autoindex

        if (client._location.getAutoIndex() == true)
        {
            // use cgi using opendir,readdir to create a dynamic html page
        }
        else
        {
            throw ErrorCodeClientException(client, 404, "couldn't find index page", client._location.getErrorCodesWithPage());
        }
    } else if (S_ISREG(status.st_mode))
    {
        if (access(client._path.data(), R_OK) == -1)
        {
            cerr << "locateRequestedFile: " << strerror(errno) << endl;
        }
        cout << "\t" << client._path << endl;
    }
    else
    {
        throw ErrorCodeClientException(client, 404, "Forbidden: Not a regular file or directory", client._location.getErrorCodesWithPage());
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

void    HttpRequest::GET(Client &client)
{
    locateRequestedFile(client);

    int fd = open(client._path.data(), R_OK);
    if (fd == -1)
        throw RunServers::ClientException("open failed");

    FileDescriptor::setFD(fd);
    size_t fileSize = getFileLength(client._path);
    // cout << _headerBlock << endl;
    string responseStr = HttpResponse(200, client._path, fileSize);
    auto handle = make_unique<HandleTransfer>(client, fd, responseStr, fileSize);
    RunServers::insertHandleTransfer(move(handle));

}

void HttpRequest::getContentLength(Client &client, const string_view content)
{
    if (content.empty())
    {
        throw RunServers::LengthRequiredException("Content-Length header is empty.");
    }
    for (size_t i = 0; i < content.size(); ++i)
    {
        if (!isdigit(static_cast<unsigned char>(content[i])))
            throw RunServers::ClientException("Content-Length contains non-digit characters.");
    }
    long long value;
    try
    {
        value = stoll(content.data());

        if (value < 0)
            throw RunServers::ClientException("Content-Length cannot be negative.");

        if (static_cast<size_t>(value) > client._location.getClientBodySize())
            throw RunServers::ClientException("Content-Length exceeds maximum allowed."); // (413, "Payload Too Large");

        if (value == 0)
            throw RunServers::ClientException("Content-Length cannot be zero.");
    }
    catch (const invalid_argument &)
    {
        throw RunServers::ClientException("Content-Length is invalid (not a number).");
    }
    catch (const out_of_range &)
    {
        throw RunServers::ClientException("Content-Length value is out of range.");
    }
    client._contentLength = static_cast<size_t>(value);
}

void    HttpRequest::handleRequest(Client &client)
{
    // validateHEAD(_client);

    // std::cout << escape_special_chars(client._header) << std::endl;
    if (client._path == "/favicon.ico")
	{
        client._path = "/favicon.svg";
	}
    if (client._headerFields.find("Host") == client._headerFields.end())
        throw RunServers::ClientException("Missing Host header");
    // else if (it->second != "127.0.1.1:8080")
    //     throw Server::ClientException("Invalid Host header: expected 127.0.1.1:8080, got " + string(it->second));
    if (client._method == "GET")
    {
        GET(client);
    }
    else if (client._method == "POST")
    {
        // cout << _headerBlock << _body << endl;
        // Submitting a form (e.g., login, registration, contact form)
        // Uploading a file (e.g., images, documents)
        // Sending JSON data (e.g., for APIs)
        // Creating a new resource (e.g., adding a new item to a database)
        // Triggering an action (e.g., starting a job, sending an email)
        POST(client);
        // cout << _contentType << '\t' << _bodyBoundary << endl;
    }
    // else
    // {

    // }
    auto it = client._headerFields.find("Connection");
    if (it == client._headerFields.end() || it->second == "keep-alive")
    {
		client._keepAlive = true;
        // FileDescriptor::addClientFD(client._fd);
    }
	else
		client._keepAlive = false;
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
    response << "Content-Type: " << getMimeType(path) << "\r\n";
    response << "Content-Length: " << fileSize << "\r\n";
    response << "\r\n";

    return response.str();

}