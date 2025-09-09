#include "ErrorCodeClientException.hpp"
#include "HttpRequest.hpp"
#include "RunServer.hpp"
#include "Constants.hpp"

#include "Logger.hpp"
namespace
{
    void appendToHeader(Client &client, const char *buff, size_t receivedBytes);
    bool detectHeaderEnd(Client &client, size_t &headerEnd);
    void checkNullBytes(const string &header, Client &client);
    void parseHeaders(Client &client);
    string_view trimWhiteSpace(string_view sv);
    void handleConnectionHeader(Client &client);
    bool handlePost(Client &client);
}

bool HttpRequest::parseHttpHeader(Client &client, const char *buff, size_t receivedBytes)
{
    appendToHeader(client, buff, receivedBytes);

    size_t headerEnd;
    if (!detectHeaderEnd(client, headerEnd))
        return false;

    checkNullBytes(client._header, client);

    parseHeaders(client);
    validateHEAD(client);

    handleConnectionHeader(client);

    auto cookie = client._headerFields.find("Cookie");
    char buffer[UUID_SIZE];
    if (cookie == client._headerFields.end()) {
        Logger::log(DEBUG, "No session cookie found, creating a new one."); //testlog
        RunServers::setSessionData(generateUuid(buffer), SessionData{});
        client._sessionId = string(buffer);
    }
    else {
        if (cookie->second.substr(0, 11) == "session_id=") {
            // Logger::log(DEBUG, "Found session cookie: " + string(cookie->second).substr(11)); //testlog
            // if (RunServers::sessionsExist(string(cookie->second).substr(11)))
            //     Logger::log(DEBUG, "Session cookie found: " + string(cookie->second).substr(11)); //testlog
            if (!RunServers::sessionsExist(string(cookie->second).substr(11))) {
                RunServers::setSessionData(generateUuid(buffer), SessionData{});
                // Logger::log(DEBUG, "Created new session cookie: " + string(buffer)); //testlog
                client._sessionId = string(buffer);
            }
        }
        else { // errorcodeclient?
            // Logger::log(DEBUG, "Invalid session cookie format, creating a new one."); //testlog
            RunServers::setSessionData(generateUuid(buffer), SessionData{});
            client._sessionId = string(buffer);
        }
    }
    

    checkNullBytes(client._header, client);
    if (client._method == "POST") {
        HttpRequest::getContentLength(client);
        return handlePost(client);
    }

    client._headerParseState = REQUEST_READY;
    return true;
}

bool HttpRequest::parseHttpBody(Client &client, const char *buff, size_t receivedBytes)
{
    client._body.append(buff, receivedBytes);
    if (client._contentType == "multipart/form-data")
    {
        client._bodyEnd = client._body.find(CRLF2);
        if (client._bodyEnd == string::npos)
            return false;
    }
    else { // application/x-www-form-urlencoded or others
        if (client._body.size() < client._contentLength) {
            return false;
        }
        else if (client._body.size() > client._contentLength)
            throw ErrorCodeClientException(client, 400, "Body size exceeds Content-Length");
    }
    client._headerParseState = REQUEST_READY;
    return true;
}

namespace
{
    inline void appendToHeader(Client &client, const char *buff, size_t receivedBytes)
    {
        client._header.append(buff, receivedBytes);
        if (client._header.size() > RunServers::getRamBufferLimit())
            throw ErrorCodeClientException(client, 413, "Header size sent by client larger then ramBufferLimit");
    }

    bool detectHeaderEnd(Client &client, size_t &headerEnd)
    {
        headerEnd = client._header.find(CRLF2);
        if (headerEnd == string::npos)
            return false;
        client._body = client._header.substr(headerEnd + CRLF2_LEN);
        client._header = client._header.erase(headerEnd + CRLF2_LEN);
        return true;
    }

    inline void checkNullBytes(const string &header, Client &client)
    {
        size_t pos = header.find('\0');
        if (pos != string::npos)
            throw ErrorCodeClientException(client, 400, "Null terminator found in header request at index: " + to_string(pos));
    }

    void parseHeaders(Client &client)
    {
        size_t start = 0;
        while (start < client._header.size()) {
            size_t end = client._header.find(CRLF, start);
            if (end == string::npos)
                throw ErrorCodeClientException(client, 400, "Malformed HTTP request: header line not properly terminated");

            string_view line(&client._header[start], end - start);
            if (line.empty())
                break; // End of headers

            size_t colon = line.find(':');
            if (colon != string_view::npos) {
                // Extract key and value as string_views
                string_view key = trimWhiteSpace(line.substr(0, colon));
                string_view value = trimWhiteSpace(line.substr(colon + 1));
                client._headerFields[string(key)] = value;
            }
            start = end + 2;
        }
    }

    string_view trimWhiteSpace(string_view sv)
    {
        const char *ws = " \t";
        size_t first = sv.find_first_not_of(ws);
        if (first == string_view::npos)
            return {}; // all whitespace
        size_t last = sv.find_last_not_of(ws);
        return sv.substr(first, last - first + 1);
    }

    void handleConnectionHeader(Client &client)
    {
        auto connectionHeader = client._headerFields.find("Connection");
        if (connectionHeader != client._headerFields.end()) {
            string_view connValue = connectionHeader->second;
            if (connValue == "close")
                client._keepAlive = false;
            else if (connValue == "keep-alive")
                client._keepAlive = true;
            else
                throw ErrorCodeClientException(client, 400, "Invalid Connection header value: " + string(connValue));
        }
    }

    bool handlePost(Client &client)
    {
        HttpRequest::getContentType(client);
        auto transferEncodingHeader = client._headerFields.find("Transfer-Encoding");
        if (transferEncodingHeader != client._headerFields.end() &&
            transferEncodingHeader->second == "chunked") {
            client._headerParseState = BODY_CHUNKED;
            return (client._body.size() > 0 ? true : false);
        }
        client._headerParseState = BODY_AWAITING;
        if (client._contentType == "multipart/form-data")
        {
            client._bodyEnd = client._body.find(CRLF2);
            if (client._bodyEnd == string::npos)
                return false;
        }
        else if (client._contentType == "application/x-www-form-urlencoded")
        {
            if (client._body.size() > client._contentLength)
                throw ErrorCodeClientException(client, 400, "Body size exceeds Content-Length");
            else if (client._body.size() < client._contentLength)
                return false;
        }
        client._headerParseState = REQUEST_READY;
        return true;
    }
}
