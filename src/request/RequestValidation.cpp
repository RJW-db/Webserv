#include <iostream>
#include <FileDescriptor.hpp>
#include <HttpRequest.hpp>
#include <RunServer.hpp>

// Static Functions
static string percentDecode(const string& input);
static bool isValidAndNormalizeRequestPath(Client &client);
static bool pathContainsInvalidCharacters(const string &path);
static vector<string_view> splitPathSegments(const string &path);
static vector<string_view> normalizeSegments(const vector<string_view> &segments);
static void    joinSegmentsToPath(string &path, const vector<string_view> &segments);
static void    validatePathAndSegmentLengths(Client &client, const vector<string_view> &segments);
static uint8_t checkAllowedMethod(string &method, uint8_t allowedMethods);

void    HttpRequest::validateHEAD(Client &client)
{
    istringstream headStream(client._header);
    headStream >> client._method >> client._requestPath >> client._version;

    client._requestPath = percentDecode(client._requestPath);
    if (client._method.empty() || client._requestPath.empty() || client._version.empty())
    {
        throw ErrorCodeClientException(client, 400, "Malformed request line");
    }

    size_t queryPos = client._requestPath.find('?');
    if (queryPos != string::npos)
    {
        // std::cout << client._requestPath << std::endl; //testcout
        client._queryString = client._requestPath.substr(queryPos + 1);
        client._requestPath = client._requestPath.substr(0, queryPos);
        // std::cout << client._requestPath << std::endl; //testcout
        // std::cout << client._queryString << std::endl; //testcout
        // exit(0);
    }

    // locateRequestedFile(client);
    struct stat status;

    string reqPath = '.' + client._requestPath;
    if (stat(reqPath.data(), &status) == -1)
    {
        throw ErrorCodeClientException(client, 404, "Couldn't find file: " + reqPath + ", because: " + string(strerror(errno)));
    }
    if (S_ISDIR(status.st_mode) == false &&
       (S_ISREG(status.st_mode) == false ||
        access(reqPath.data(), R_OK) != 0))
    {
        throw ErrorCodeClientException(client, 404, "Forbidden: Not a regular file or directory");
    }

    RunServers::setServer(client);
    RunServers::setLocation(client);
    client._useMethod = checkAllowedMethod(client._method, client._location.getAllowedMethods());
    if (client._useMethod == 0)
    {
        throw ErrorCodeClientException(client, 405, "Method not allowed: " + client._method);
    }

    if (isValidAndNormalizeRequestPath(client) == false)
    {
        throw ErrorCodeClientException(client, 400, "Invalid HTTP path: " + client._requestPath);
    }
    if (client._version != "HTTP/1.1")
    {
        throw ErrorCodeClientException(client, 400, "Invalid version: " + client._version);
    }

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

static uint8_t checkAllowedMethod(string &method, uint8_t allowedMethods)
{
    if (method == "HEAD" && allowedMethods & 1)
        return 1;
    if (method == "GET" && allowedMethods & 2)
        return 2;
    if (method == "POST" && allowedMethods & 4)
        return 4;
    if (method == "DELETE" && allowedMethods & 8)
        return 8;
    return 0;
}
