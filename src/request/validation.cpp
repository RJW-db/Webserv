#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include "ErrorCodeClientException.hpp"
#include "FileDescriptor.hpp"
#include "HttpRequest.hpp"
#include "RunServer.hpp"
#include "Logger.hpp"

namespace
{
    void                 parseRequestPath(Client &client, string &requestPath);
    uint8_t              checkAllowedMethod(Client &client);
    string               percentDecode(const string& input);
    bool                 isValidAndNormalizeRequestPath(Client &client, string &requestPath);
    bool                 pathContainsInvalidCharacters(const string &path);
    vector<string_view>  splitPathSegments(const string &path);
    vector<string_view>  normalizeSegments(const vector<string_view> &segments);
    void                 joinSegmentsToPath(string &path, const vector<string_view> &segments);
    void                 validatePathAndSegmentLengths(Client &client, const vector<string_view> &segments);
    void                 decodeSafeFilenameChars(Client &client);
    void                 findIndexFile(Client &client, struct stat &status);
    void                 validateResourceAccess(Client &client);
    void                 detectCgiRequest(Client &client);
}

void    HttpRequest::validateHEAD(Client &client)
{
    istringstream headStream(client._header);
    headStream >> client._method >> client._requestPath >> client._version;
    if (client._method.empty() || client._version.empty())
        throw ErrorCodeClientException(client, 400, "Malformed request line");
    parseRequestPath(client, client._requestPath);
    RunServers::setServerFromListener(client);
    RunServers::setLocation(client);

    client._useMethod = checkAllowedMethod(client);

    if (client._version != "HTTP/1.1")
        throw ErrorCodeClientException(client, 400, "Invalid version: " + client._version);
    
    client._rootPath = client._location.getRoot() + string(client._requestPath);
    decodeSafeFilenameChars(client);
    if (client._requestPath == "/upload/list")
    {
        client._requestUpload = true;
        client._requestPath = "/upload";
    }

    if ((client._useMethod & METHOD_GET && client._requestPath == "/get-theme") ||
        (client._useMethod & METHOD_POST && client._requestPath == "/set-theme")) {
        return ;
    }
    validateResourceAccess(client);
}

namespace
{
    void parseRequestPath(Client &client, string &requestPath)
    {
        requestPath = percentDecode(requestPath);
        size_t queryPos = requestPath.find('?');
        if (queryPos != string::npos) {
            client._queryString = requestPath.substr(queryPos + 1);
            requestPath = requestPath.substr(0, queryPos);
        }
        
        if (isValidAndNormalizeRequestPath(client, requestPath) == false)
            throw ErrorCodeClientException(client, 400, "Invalid HTTP path: " + requestPath);

        size_t faviconIndex = requestPath.find("/favicon.ico");
        if (faviconIndex != string::npos)
            requestPath = requestPath.substr(0, faviconIndex) + "/favicon.svg";
    }

    uint8_t checkAllowedMethod(Client &client)
    {
        string &method = client._method;
        uint8_t allowedMethods = client._location.getAllowedMethods();
    
        if (method == "HEAD" && allowedMethods & METHOD_HEAD)
            return METHOD_HEAD;
        if (method == "GET" && allowedMethods & METHOD_GET)
            return METHOD_GET;
        if (method == "POST" && allowedMethods & METHOD_POST)
            return METHOD_POST;
        if (method == "DELETE" && allowedMethods & METHOD_DELETE)
            return METHOD_DELETE;

        if (method == "HEAD" || method == "GET" || method == "POST" || method == "DELETE" ||
            method == "PUT" || method == "PATCH" || method == "OPTIONS" || 
            method == "TRACE" || method == "CONNECT") {
            throw ErrorCodeClientException(client, 405, "Method not allowed: " + client._method);
        }
        throw ErrorCodeClientException(client, 400, "Invalid method: " + client._method);
    }

    string percentDecode(const string& input)
    {
        string result;
        for (size_t i = 0; i < input.length(); ++i) {
            if (input[i] == '%' && i + 2 < input.length() &&
                isxdigit(input[i + 1]) && isxdigit(input[i + 2]))
            {
                string hex = input.substr(i + 1, 2);
                // stoi doesn't need a protection because of isxdigit
                result += static_cast<char>(stoi(hex.c_str(), nullptr, 16));
                i += 2;
            }
            else
                result += input[i];
        }
        return result;
    }

    bool isValidAndNormalizeRequestPath(Client &client, string &requestPath)
    {
        if (requestPath.empty() || requestPath.data()[0] != '/' ||
            pathContainsInvalidCharacters(requestPath)) {
            return false;
        }
        vector<string_view> pathSegments = splitPathSegments(requestPath);
        vector<string_view> normalizedSegments = normalizeSegments(pathSegments);
        joinSegmentsToPath(requestPath, normalizedSegments);
        validatePathAndSegmentLengths(client, normalizedSegments);
        return true;
    }

    bool pathContainsInvalidCharacters(const string &path)
    {
        for (size_t i = 0; i < path.size(); ++i) {
            char c = path[i];
            if (!isprint(static_cast<unsigned char>(c)) || c == '/' || c == '\\' || c == '\0')
                return false;
        }
        return true;
    }

    // Helper: Split path into segments, skipping empty ones
    vector<string_view> splitPathSegments(const string &path)
    {
        vector<string_view> segments;
        size_t start = 0, end;
        while ((end = path.find('/', start)) != string::npos) {
            if (end != start)
                segments.push_back(string_view(path).substr(start, end - start));
            start = end + 1;
        }
        if (start < path.size())
            segments.push_back(string_view(path).substr(start));
        return segments;
    }

    // Helper: Normalize segments, handling ".." and preventing traversal above root
    vector<string_view> normalizeSegments(const vector<string_view> &segments)
    {
        vector<string_view> normalized;
        for (size_t i = 0; i < segments.size(); ++i) {
            if (segments[i] == "." || segments[i].empty())
                continue;
            if (segments[i] == "..") {
                if (normalized.empty())
                    continue; // Skip leading ".." (browser behavior)
                normalized.pop_back();
            }
            else
                normalized.push_back(segments[i]);
        }
        return normalized;
    }

    void    joinSegmentsToPath(string &path, const vector<string_view> &segments)
    {
        string newPath = "/";
        for (size_t i = 0; i < segments.size(); ++i) {
            newPath += segments[i];
            if (i + 1 < segments.size())
                newPath += "/";
        }
        if (path.back() == '/' && newPath.size() > 1)
            newPath += "/";
        path = newPath;
    }

    void    validatePathAndSegmentLengths(Client &client, const vector<string_view> &segments)
    {
        if (client._requestPath.size() > PATH_MAX)
            throw ErrorCodeClientException(client, 414, "URI too long");

        for (auto segment : segments)
            if (segment.size() > NAME_MAX)
                throw ErrorCodeClientException(client, 414, "URI too long");
    }

    void decodeSafeFilenameChars(Client &client)
    {
        const map<string, string> specialChars = {
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

    void validateResourceAccess(Client &client)
    {
        struct stat status;
        string root = client._location.getRoot();
        if (!root.empty() && root.back() == '/')
            root.pop_back();
        string reqPath = root + client._requestPath;
        if (stat(reqPath.data(), &status) == -1)
            throw ErrorCodeClientException(client, 404, "Couldn't find file: " + reqPath + ", because: " + string(strerror(errno)));

        if (S_ISDIR(status.st_mode) == true) {
            if (access(reqPath.data(), R_OK | X_OK) != 0)
                throw ErrorCodeClientException(client, 403, "Forbidden: No permission to access directory");
            
            if (client._requestUpload == false && client._useMethod & (METHOD_HEAD | METHOD_GET))
                findIndexFile(client, status);
        }
        else if (S_ISREG(status.st_mode) == true) {
            if (access(reqPath.data(), R_OK) != 0)
                throw ErrorCodeClientException(client, 403, "Forbidden: No permission to read file");
            detectCgiRequest(client);
            if (client._useMethod & (METHOD_HEAD | METHOD_GET))
                client._filenamePath = reqPath;
        }
        else
            throw ErrorCodeClientException(client, 404, "Not a regular file or directory");
    }

    void findIndexFile(Client &client, struct stat &status)
    {
        for (const string &indexPage : client._location.getIndexPage()) {
            if (stat(indexPage.data(), &status) == 0) {
                if (S_ISDIR(status.st_mode) == true ||
                    S_ISREG(status.st_mode) == false)
                    continue;
                if (access(indexPage.data(), R_OK) == -1) {
                    Logger::log(WARN, "Access error", '-', "Access failed: ", strerror(errno));
                    continue;
                }
                client._filenamePath = indexPage;
                return;
            }
        }
        
        if (client._location.getAutoIndex() == true)
            client._isAutoIndex = true;
        else
            throw ErrorCodeClientException(client, 404, "couldn't find index page");
    }

    void detectCgiRequest(Client &client) {
        size_t filenamePos = client._requestPath.find_last_of('/');
        string_view filename(client._requestPath.data() + filenamePos + 1);
        if (client._location.isCgiFile(filename) == true)
            client._isCgi = true;
        else if (!client._location.getExtension().empty())
            throw ErrorCodeClientException(client, 400, "request without correct cgi extension");
    }
}
