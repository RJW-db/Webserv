#include "ErrorCodeClientException.hpp"
#include "HttpRequest.hpp"
#include "RunServer.hpp"
#include "Constants.hpp"
#include "utils.hpp"

namespace
{
    void appendToHeader(Client &client, const char *buff, size_t receivedBytes);
    bool detectHeaderEnd(Client &client, size_t &headerEnd);
    void checkNullBytes(const string &header, Client &client);
    void parseHeaders(Client &client);
    string_view trimWhiteSpace(string_view sv);
    void handleConnectionHeader(Client &client);
    void assignOrCreateSessionId(Client &client);
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
    assignOrCreateSessionId(client);

    checkNullBytes(client._header, client);
    if (client._method == "POST")
        return handlePost(client);

    client._headerParseState = REQUEST_READY;
    return true;
}

namespace
{
    inline void appendToHeader(Client &client, const char *buff, size_t receivedBytes)
    {
        client._header.append(buff, receivedBytes);
        if (client._header.size() > RunServers::getRamBufferLimit())
            throw ErrorCodeClientException(client, PAYLOAD_TOO_LARGE, "Header size sent by client larger then ramBufferLimit");
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
            throw ErrorCodeClientException(client, BAD_REQUEST, "Null terminator found in header request at index: " + to_string(pos));
    }

    void parseHeaders(Client &client)
    {
        size_t start = 0;
        while (start < client._header.size()) {
            size_t end = client._header.find(CRLF, start);
            if (end == string::npos)
                throw ErrorCodeClientException(client, BAD_REQUEST, "Malformed HTTP request: header line not properly terminated");

            string_view line(&client._header[start], end - start);
            if (line.empty())
                break; // End of headers

            size_t colon = line.find(':');
            if (colon != string_view::npos) {
                // Convert header name to lowercase IN PLACE
                for (size_t i = start; i < start + colon; ++i) {
                    client._header[i] = static_cast<char>(std::tolower(client._header[i]));
                }
                // Extract key and value as string_views
                string_view key = trimWhiteSpace(line.substr(0, colon));
                string_view value = trimWhiteSpace(line.substr(colon + 1));
                client._headerFields[key] = value;
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
        if (RunServers::getErrorOccurred() != SERVER_GOOD)
            return;
        auto connectionHeader = client._headerFields.find("connection");
        if (connectionHeader != client._headerFields.end()) {
            string_view connValue = connectionHeader->second;
            if (connValue == "close")
                client._keepAlive = false;
            else if (connValue == "keep-alive")
                client._keepAlive = true;
            else
                throw ErrorCodeClientException(client, BAD_REQUEST, "Invalid Connection header value: " + string(connValue));
        }
    }
    void assignOrCreateSessionId(Client &client)
    {
        auto cookie = client._headerFields.find("cookie");
        char sessionId[ID_SIZE];

        bool validSessionCookie = false;
        string cookieSessionId;
        if (cookie != client._headerFields.end() &&
            cookie->second.substr(0, 11) == "session_id=" &&
            cookie->second.size() >= 12) {
            cookieSessionId = string(cookie->second).substr(11);
            if (RunServers::sessionsExist(cookieSessionId))
                validSessionCookie = true;
        }

        if (validSessionCookie)
            client._sessionId = move(cookieSessionId);
        else {
            RunServers::setSessionData(generateSessionIdCookie(sessionId), SessionData{});
            client._sessionId = string(sessionId);
        }
    }

    bool handlePost(Client &client)
    {
        HttpRequest::getContentType(client);
        auto transferEncodingHeader = client._headerFields.find("transfer-encoding");
        if (transferEncodingHeader != client._headerFields.end() &&
            transferEncodingHeader->second == "chunked") {
            transferEncodingHeader = client._headerFields.find("content-length");
            if (transferEncodingHeader != client._headerFields.end())
                throw ErrorCodeClientException(client, BAD_REQUEST, "Content-Length shouldn't be present in a chunked request");
            client._headerParseState = BODY_CHUNKED;
            return (client._body.size() > 0 ? true : false);
        }

        HttpRequest::getContentLength(client);
        if (client._contentType == FORM_URLENCODED)
        {
            if (client._body.size() > client._contentLength)
                throw ErrorCodeClientException(client, BAD_REQUEST, "Body size exceeds Content-Length");
            else if (client._body.size() < client._contentLength)
                return false;
        }
        client._headerParseState = REQUEST_READY;
        return true;
    }
}
