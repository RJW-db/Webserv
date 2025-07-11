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
    if (findDelimiter(client, headerEnd, receivedBytes) == false)
        return false;

    client._headerParseState = true;
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
    // std::cout << escape_special_chars(client._header) << std::endl; //testcout

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

    if (client._method == "POST")
    {
        it = client._headerFields.find("Transfer-Encoding");
        if (it != client._headerFields.end() && it->second == "chunked")
        {
            client._headerParseState = BODY_CHUNKED;
            return true;
        }
        client._headerParseState = BODY_AWAITING;
        client._bodyEnd = client._body.find("\r\n\r\n");
        if (findDelimiter(client, client._bodyEnd, receivedBytes) == false)
            return false;
        return true;
    }
    client._headerParseState = BODY_READY;
    return true;
}

bool HttpRequest::parseHttpBody(Client &client, const char *buff, size_t receivedBytes)
{
    client._body.append(buff, receivedBytes);
    client._bodyEnd = client._body.find("\r\n\r\n");
    if (findDelimiter(client, client._bodyEnd, receivedBytes) == false)
        return false;
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
            client._rootPath = indexPage; // found index
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
        findIndexFile(client, status);
    else if (S_ISREG(status.st_mode))
    {
        if (access(client._rootPath.data(), R_OK) == -1)
        {
            cerr << "locateRequestedFile: " << strerror(errno) << endl;
        }
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

    int fd = open(client._rootPath.data(), R_OK);
    if (fd == -1)
        throw RunServers::ClientException("open failed");

    FileDescriptor::setFD(fd);
    size_t fileSize = getFileLength(client._rootPath);
    string responseStr = HttpResponse(client, 200, client._rootPath, fileSize);
    auto handle = make_unique<HandleTransfer>(client, fd, responseStr, fileSize);
    RunServers::insertHandleTransfer(move(handle));
}

// void HttpRequest::getContentLength(Client &client)
// {
//     auto contentLength = client._headerFields.find("Content-Length");
//     if (contentLength == client._headerFields.end())
//         throw RunServers::ClientException("Broken POST request");
//     const string_view content = contentLength->second;
//     if (content.empty())
//     {
//         throw RunServers::LengthRequiredException("Content-Length header is empty.");
//     }
//     for (size_t i = 0; i < content.size(); ++i)
//     {
//         if (!isdigit(static_cast<unsigned char>(content[i])))
//             throw RunServers::ClientException("Content-Length contains non-digit characters.");
//     }
//     long long value;
//     try
//     {
//         value = stoll(content.data());
//     }
//     catch (const invalid_argument &)
//     {
//         throw RunServers::ClientException("Content-Length is invalid (not a number).");
//     }
//     catch (const out_of_range &)
//     {
//         throw RunServers::ClientException("Content-Length value is out of range.");
//     }

//     if (value < 0)
//         throw RunServers::ClientException("Content-Length cannot be negative.");

//     if (static_cast<size_t>(value) > client._location.getClientBodySize())
//         throw ErrorCodeClientException(client, 413, "Content-Length exceeds maximum allowed: " + to_string(value)); // (413, "Payload Too Large");

//     if (value == 0)
//         throw RunServers::ClientException("Content-Length cannot be zero.");
//     client._contentLength = static_cast<size_t>(value);
// }

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
    if (client._headerFields.find("Host") == client._headerFields.end())
        throw ErrorCodeClientException(client, 400, "Host header is missing in request: " + client._header);
    // else if (it->second != "127.0.1.1:8080")
    //     throw Server::ClientException("Invalid Host header: expected 127.0.1.1:8080, got " + string(it->second));
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
            case BODY_READY:
                processHttpBody(client);
                break;
            
            case BODY_CHUNKED:
                {
                    // std::cout << escape_special_chars(client._header) << std::endl; //testcout
                    // std::cout << "body " << escape_special_chars(client._body) << " <" << std::endl; //testcout
                    handleChunks(client);
                    // std::cout << "okeeeee\n\n" << std::endl; //testcout
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
