#include <iostream>
#include <FileDescriptor.hpp>
#include <HttpRequest.hpp>
#include <RunServer.hpp>

// Static Functions
static bool isValidAndNormalizeRequestPath(Client &client);
static bool pathContainsInvalidCharacters(const string &path);
static vector<string_view> splitPathSegments(const string &path);
static vector<string_view> normalizeSegments(const vector<string_view> &segments, Client &client);
static void    joinSegmentsToPath(string &path, const vector<string_view> &segments);
static void    validatePathAndSegmentLengths(Client &client, const vector<string_view> &segments);

void    HttpRequest::validateHEAD(Client &client)
{
    istringstream headStream(client._header);
    headStream >> client._method >> client._requestPath >> client._version;

    if (client._method.empty() || client._requestPath.empty() || client._version.empty()) {
        throw ErrorCodeClientException(client, 400, "Malformed request line");
    }
    RunServers::setServer(client);
    RunServers::setLocation(client);
    if (/* client._method != "HEAD" &&  */client._method != "GET" && client._method != "POST" && client._method != "DELETE")
    {
        throw ErrorCodeClientException(client, 405, "Invalid HTTP method: " + client._method);
            // sendErrorResponse(client._fd, "400 Bad Request");
    }
    if (isValidAndNormalizeRequestPath(client) == false)
    {
        throw ErrorCodeClientException(client, 400, "Invalid HTTP path: " + client._requestPath);
    }
    std::cout << client._requestPath << std::endl; //testcout
    if (client._version != "HTTP/1.1")
    {
        throw ErrorCodeClientException(client, 400, "Invalid version: " + client._version);
    }
}

static bool isValidAndNormalizeRequestPath(Client &client)
{
    if (client._requestPath.empty() || client._requestPath.data()[0] != '/' ||
        pathContainsInvalidCharacters(client._requestPath))
    {
        return false;
    }

    vector<string_view> pathSegments = splitPathSegments(client._requestPath);
    vector<string_view> normalizedSegments = normalizeSegments(pathSegments, client);
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
static vector<string_view> normalizeSegments(const vector<string_view> &segments, Client &client)
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
