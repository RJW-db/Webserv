#include <iostream>
#include <FileDescriptor.hpp>
#include <HttpRequest.hpp>
#include <RunServer.hpp>
#include <Client.hpp>

#include <HandleTransfer.hpp>
#include <ErrorCodeClientException.hpp>
#include <unordered_set>
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
#include <stdlib.h> // callod
#ifdef __linux__
#include <sys/epoll.h>
#endif
#include <sys/socket.h>

#include <dirent.h>

#include <signal.h>

#include <cstdio> // remove

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <string_view>

bool HttpRequest::parseHttpHeader(Client &client, const char *buff, size_t receivedBytes)
{
    client._header.append(buff, receivedBytes); // can fail, need to call cleanupClient
    size_t headerEnd = client._header.find("\r\n\r\n");
    if (headerEnd == string::npos)
        return false;

    client._body = client._header.substr(headerEnd + 4);      // can fail, need to call cleanupClient
    client._header = client._header.substr(0, headerEnd + 4); // can fail, need to call cleanupClient

    size_t ConnectionIndex = client._header.find("Connection:");
    if (ConnectionIndex != string::npos)
    {
        if (client._header.find("close\r\n", ConnectionIndex) != string::npos)
            client._keepAlive = false;
        else if (client._header.find("keep-alive\r\n", ConnectionIndex) != string::npos)
            client._keepAlive = true;
        else
            throw ErrorCodeClientException(client, 400, "Invalid Connection header value: " + client._header.substr(ConnectionIndex));
    }
    std::cout << "only once\n" << std::endl; //testcout
    HttpRequest::validateHEAD(client); // TODO cleanupClient
    HttpRequest::parseHeaders(client); // TODO cleanupClient
    // Check if there is a null character in buff
    if (client._header.find('\0') != string::npos)
       throw ErrorCodeClientException(client, 400, "Null bytes not allowed in HTTP request");
    if (client._location.getRoot().back() == '/')
        client._rootPath = client._location.getRoot() + string(client._requestPath).substr(1);
    else
        client._rootPath = client._location.getRoot() + string(client._requestPath);
    decodeSafeFilenameChars(client);

    auto it = client._headerFields.find("Connection");
    if (it != client._headerFields.end() && it->second == "close")
        client._keepAlive = false;
    if (client._method == "POST")
    {
        
        // HttpRequest::getContentLength(client);
        HttpRequest::getContentType(client); // TODO return isn't used at all
        it = client._headerFields.find("Transfer-Encoding");
        if (it != client._headerFields.end() && it->second == "chunked")
        {
            client._headerParseState = BODY_CHUNKED;
            return (client._body.size() > 0 ? true : false);
        }
        client._headerParseState = BODY_AWAITING;
        client._bodyEnd = client._body.find("\r\n\r\n");
        if (client._bodyEnd == string::npos)
            return false;
        client._headerParseState = REQUEST_READY;
        return true;
    }
    client._headerParseState = REQUEST_READY;
    return true;
}

bool HttpRequest::parseHttpBody(Client &client, const char *buff, size_t receivedBytes)
{
    client._body.append(buff, receivedBytes);
    client._bodyEnd = client._body.find("\r\n\r\n");
    if (client._bodyEnd == string::npos)
        return false;
    client._headerParseState = REQUEST_READY;
    return true;
}

static string_view trimWhiteSpace(string_view sv)
{
    const char *ws = " \t";
    size_t first = sv.find_first_not_of(ws);
    if (first == string_view::npos)
        return {}; // all whitespace
    size_t last = sv.find_last_not_of(ws);
    return sv.substr(first, last - first + 1);
}

void HttpRequest::parseHeaders(Client &client)
{
    size_t start = 0;
    while (start < client._header.size())
    {
        size_t end = client._header.find("\r\n", start);
        if (end == string::npos)
            throw ErrorCodeClientException(client, 400, "Malformed HTTP request: header line not properly terminated");

        string_view line(&client._header[start], end - start);
        if (line.empty())
            break; // End of headers

        size_t colon = line.find(':');
        if (colon != string_view::npos)
        {
            // Extract key and value as string_views
            string_view key = trimWhiteSpace(line.substr(0, colon));
            string_view value = trimWhiteSpace(line.substr(colon + 1));
            client._headerFields[string(key)] = value;
            // cout << "\tkey\t" << key << "\t" << value << endl;
        }
        start = end + 2;
    }
}

void HttpRequest::decodeSafeFilenameChars(Client &client)
{
    static const map<string, string> specialChars = {
        {"%20", " "},
        {"%21", "!"},
        {"%24", "$"},
        {"%25", "%"},
        {"%26", "&"}};
    for (pair<string, string> pair : specialChars)
    {
        size_t pos;
        while ((pos = client._rootPath.find(pair.first)) != string::npos)
            client._rootPath.replace(pos, 3, pair.second);
    }
    if (client._rootPath.find('%') != string::npos)
        throw ErrorCodeClientException(client, 400, "bad filename given by client:" + client._rootPath);
}

void HttpRequest::findIndexFile(Client &client, struct stat &status)
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
            client._filenamePath = indexPage;
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
        throw ErrorCodeClientException(client, 404, "couldn't find index page");
    }
}

void HttpRequest::locateRequestedFile(Client &client)
{
    struct stat status;

    if (stat(client._rootPath.data(), &status) == -1)
    {
        throw ErrorCodeClientException(client, 404, "Couldn't find file: " + client._rootPath + ", because: " + string(strerror(errno)));
    }
    if (S_ISDIR(status.st_mode))
    {
        findIndexFile(client, status);
    }
    else if (S_ISREG(status.st_mode))
    {
        if (access(client._rootPath.data(), R_OK) == -1)
        {
            cerr << "locateRequestedFile: " << strerror(errno) << endl;
        }
		client._filenamePath = client._rootPath;
    }
    else
    {
        throw ErrorCodeClientException(client, 404, "Forbidden: Not a regular file or directory");
    }
}

string HttpRequest::getMimeType(string &path)
{
    static const map<string, string> mimeTypes = {
        {"html", "text/html"},
        {"htm", "text/html"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"svg", "image/svg+xml"},
        {"ico", "image/x-icon"},
        {"pdf", "application/pdf"},
        {"txt", "text/plain"},
        {"mp4", "video/mp4"},
        {"webm", "video/webm"},
        {"xml", "application/xml"}};

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

void HttpRequest::GET(Client &client)
{
    locateRequestedFile(client);

    int fd = open(client._filenamePath.data(), R_OK);
    if (fd == -1)
        throw RunServers::ClientException("open failed");

    FileDescriptor::setFD(fd);
    size_t fileSize = getFileLength(client._filenamePath);
    string responseStr = HttpResponse(client, 200, client._filenamePath, fileSize);
    auto handle = make_unique<HandleTransfer>(client, fd, responseStr, static_cast<size_t>(fileSize));
    RunServers::insertHandleTransfer(move(handle));
}

void HttpRequest::getContentLength(Client &client)
{
    auto contentLength = client._headerFields.find("Content-Length");
    if (contentLength == client._headerFields.end())
        throw RunServers::ClientException("Broken POST request");

    const string_view content = contentLength->second;
    try
    {
        uint64_t value = stoullSafe(content);
        if (static_cast<size_t>(value) > client._location.getClientBodySize())
            throw ErrorCodeClientException(client, 413, "Content-Length exceeds maximum allowed: " + to_string(value));
        if (value == 0)
            throw RunServers::ClientException("Content-Length cannot be zero.");
        client._contentLength = static_cast<size_t>(value);
    }
    catch (const std::runtime_error &e)
    {
        throw RunServers::ClientException(e.what());
    }
}

void HttpRequest::handleRequest(Client &client)
{
    if (client._rootPath.find("/favicon.ico") != string::npos)
        client._rootPath = client._rootPath.substr(0, client._rootPath.find("/favicon.ico")) + "/favicon.svg";

    switch (client._useMethod)
    {
    case 1: // HEAD
    {
        locateRequestedFile(client);
        string response = HttpRequest::HttpResponse(client, 200, "txt", 0);
        send(client._fd, response.data(), response.size(), MSG_NOSIGNAL);
        break;
    }
    case 2: // GET
    {
        GET(client);
        break;
    }
    case 4: // POST
    {
        switch (client._headerParseState)
        {

            case REQUEST_READY:
            {
                processHttpBody(client);
                break;
            }
            case BODY_CHUNKED:
            {
                // handleChunks(client);
                client._contentLength = client._location.getClientBodySize();
                unique_ptr<HandleTransfer> handle;
                handle = make_unique<HandleTransfer>(client);
                // handle->_client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
                if (handle->handleChunkTransfer() == true)
                {
                    if (client._keepAlive == false)
                        RunServers::cleanupClient(client);
                    else
                    {
                        RunServers::clientHttpCleanup(client);
                    }
                    return;

                }
                RunServers::insertHandleTransfer(move(handle));
            }
        }
        
        break;
    }
    case 8: // DELETE
    {
        int code = 200;
        if (remove(('.' + client._requestPath).data()) == 0)
        {
            string body = "File deleted";
            string response = HttpRequest::HttpResponse(client, code, "txt", body.size());
            response += body;
            send(client._fd, response.data(), response.size(), MSG_NOSIGNAL);
            RunServers::clientHttpCleanup(client);
        }
        else
        {
            switch (errno)
            {
            case EACCES:
            case EPERM:
            case EROFS:
                code = 403; // Forbidden";
                break;
            case ENOENT:
            case ENOTDIR:
                code = 404; // Not Found";
                break;
            case EISDIR:
                // if (limit_except == Doesn't allow DELETE)
                code = 405; // Method Not Allowed";
                // else
                // code = 403; // Forbidden";
                break;
            default:
                code = 500; // Internal Server Error;

                /**
                 * Status codes already been handled
                 * 414 - ENAMETOOLONG - URI Too Long
                 */
            }
            throw ErrorCodeClientException(client, code, string("Remove failed: ") + strerror(errno));
        }
        break;
    }
    default:
        throw ErrorCodeClientException(client, 405, "Method Not Allowed: " + client._method);
    }
}

string HttpRequest::HttpResponse(Client &client, uint16_t code, string path, size_t fileSize)
{
    static const map<uint16_t, string> responseCodes = {
        {200, "OK"},
        {201, "Created"},
        {400, "Bad Request"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {413, "Payload Too Large"},
        {414, "URI Too Long"},
        {431, "Request Header Fields Too Large"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"}};

    map<uint16_t, string>::const_iterator it = responseCodes.find(code);
    if (it == responseCodes.end())
        throw runtime_error("Couldn't find code");

    ostringstream response;
    response << "HTTP/1.1 " << to_string(it->first) << ' ' << it->second << "\r\n";
    if (!path.empty())
        response << "Content-Type: " << getMimeType(path) << "\r\n";
    response << "Content-Length: " << fileSize << "\r\n";
    response << "Connection: " + string(client._keepAlive ? "keep-alive" : "close") + "\r\n";
    response << "\r\n";
    return response.str();
}
