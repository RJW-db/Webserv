#include <iostream>
#include <FileDescriptor.hpp>
#include <HttpRequest.hpp>
#include <RunServer.hpp>

// Static Functions
static uint8_t              checkAllowedMethod(string &method, uint8_t allowedMethods);
static void                 parseRequestPath(Client &client);
static string               percentDecode(const string& input);
static bool                 isValidAndNormalizeRequestPath(Client &client);
static bool                 pathContainsInvalidCharacters(const string &path);
static vector<string_view>  splitPathSegments(const string &path);
static vector<string_view>  normalizeSegments(const vector<string_view> &segments);
static void                 joinSegmentsToPath(string &path, const vector<string_view> &segments);
static void                 validatePathAndSegmentLengths(Client &client, const vector<string_view> &segments);
static void                 validateResourceAccess(Client &client);
static void                 detectCgiRequest(Client &client);

void    HttpRequest::validateHEAD(Client &client)
{
    istringstream headStream(client._header);
    headStream >> client._method >> client._requestPath >> client._version;

    if (client._method.empty() || client._version.empty())
        throw ErrorCodeClientException(client, 400, "Malformed request line");

    parseRequestPath(client);
    RunServers::setServer(client);
    RunServers::setLocation(client);
    
    client._useMethod = checkAllowedMethod(client._method, client._location.getAllowedMethods());
    if (client._useMethod == 0)
        throw ErrorCodeClientException(client, 405, "Method not allowed: " + client._method);
    
    if (client._version != "HTTP/1.1")
        throw ErrorCodeClientException(client, 400, "Invalid version: " + client._version);
    
    validateResourceAccess(client);
}

static uint8_t checkAllowedMethod(string &method, uint8_t allowedMethods)
{
    if (method == "HEAD" && allowedMethods & METHOD_HEAD)
        return 1;
    if (method == "GET" && allowedMethods & METHOD_GET)
        return 2;
    if (method == "POST" && allowedMethods & METHOD_POST)
        return 4;
    if (method == "DELETE" && allowedMethods & METHOD_DELETE)
        return 8;
    return 0;
}

static void parseRequestPath(Client &client)
{
    size_t queryPos = client._requestPath.find('?');
    if (queryPos != string::npos)
    {
        client._queryString = client._requestPath.substr(queryPos + 1);
        client._requestPath = client._requestPath.substr(0, queryPos);
    }
    
    client._requestPath = percentDecode(client._requestPath);
    
    if (isValidAndNormalizeRequestPath(client) == false)
        throw ErrorCodeClientException(client, 400, "Invalid HTTP path: " + client._requestPath);
}

static string percentDecode(const string& input)
{
    string result;
    for (size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '%' && i + 2 < input.length() &&
            isxdigit(input[i + 1]) && isxdigit(input[i + 2]))
        {
            string hex = input.substr(i + 1, 2);
            // strol doesn't need a protection because of isxdigit
            result += static_cast<char>(stoi(hex.c_str(), nullptr, 16));
            i += 2;
        }
        else
            result += input[i];
    }
    return result;
}

static bool isValidAndNormalizeRequestPath(Client &client)
{
    if (client._requestPath.empty() || client._requestPath.data()[0] != '/' ||
        pathContainsInvalidCharacters(client._requestPath))
    {
        return false;
    }
    vector<string_view> pathSegments = splitPathSegments(client._requestPath);
    vector<string_view> normalizedSegments = normalizeSegments(pathSegments);
    joinSegmentsToPath(client._requestPath, normalizedSegments);
    validatePathAndSegmentLengths(client, normalizedSegments);
    return true;
}

static bool pathContainsInvalidCharacters(const string &path)
{
    for (size_t i = 0; i < path.size(); ++i)
    {
        char c = path[i];
        if (!isprint(static_cast<unsigned char>(c)) || c == '/' || c == '\\' || c == '\0')
            return false;
    }
    return true;
}

// Helper: Split path into segments, skipping empty ones
static vector<string_view> splitPathSegments(const string &path)
{
    vector<string_view> segments;
    size_t start = 0, end;
    while ((end = path.find('/', start)) != string::npos)
    {
        if (end != start)
            segments.push_back(string_view(path).substr(start, end - start));
        start = end + 1;
    }
    if (start < path.size())
        segments.push_back(string_view(path).substr(start));
    return segments;
}

// Helper: Normalize segments, handling ".." and preventing traversal above root
static vector<string_view> normalizeSegments(const vector<string_view> &segments)
{
    vector<string_view> normalized;
    for (size_t i = 0; i < segments.size(); ++i)
    {
        if (segments[i] == "." || segments[i].empty())
            continue;
        if (segments[i] == "..")
        {
            if (normalized.empty())
                continue; // Skip leading ".." (browser behavior)
            normalized.pop_back();
        }
        else
            normalized.push_back(segments[i]);
    }
    return normalized;
}

static void    joinSegmentsToPath(string &path, const vector<string_view> &segments)
{
    string newPath = "/";
    for (size_t i = 0; i < segments.size(); ++i)
    {
        newPath += segments[i];
        if (i + 1 < segments.size())
            newPath += "/";
    }
    if (path.back() == '/' && newPath.size() > 1)
        newPath += "/";
    path = newPath;
}

static void    validatePathAndSegmentLengths(Client &client, const vector<string_view> &segments)
{
    if (client._requestPath.size() > PATH_MAX)
        throw ErrorCodeClientException(client, 414, "URI too long");

    for (auto segment : segments)
    {
        if (segment.size() > NAME_MAX)
        {
            throw ErrorCodeClientException(client, 414, "URI too long");
        }
    }
}

static void validateResourceAccess(Client &client)
{
    struct stat status;
    string root = client._location.getRoot();
    if (!root.empty() && root.back() == '/')
        root.pop_back();
    string reqPath = root + client._requestPath;
    if (stat(reqPath.data(), &status) == -1)
        throw ErrorCodeClientException(client, 404, "Couldn't find file: " + reqPath + ", because: " + string(strerror(errno)));

    if (S_ISDIR(status.st_mode) == true)    
    {
        if (access(reqPath.data(), R_OK | X_OK) != 0)
            throw ErrorCodeClientException(client, 403, "Forbidden: No permission to access directory");
        
        if (client._useMethod & (METHOD_HEAD | METHOD_GET))
            HttpRequest::findIndexFile(client, status);
    }
    else if (S_ISREG(status.st_mode) == true)
    {
        if (access(reqPath.data(), R_OK) != 0)
            throw ErrorCodeClientException(client, 403, "Forbidden: No permission to read file");
        detectCgiRequest(client);
        if (client._useMethod & (METHOD_HEAD | METHOD_GET))
        {
            client._filenamePath = reqPath;
        }
    }
    else
        throw ErrorCodeClientException(client, 404, "Not a regular file or directory");
}

static void detectCgiRequest(Client &client)
{
    size_t filenamePos = client._requestPath.find_last_of('/');
    string_view filename(client._requestPath.data() + filenamePos + 1);
    if (client._location.isCgiFile(filename) == true)
    {
        client._isCgi = true;
        std::cout << "request is cgi request" << std::endl; //testcout
    }
}