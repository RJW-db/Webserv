#include "HttpRequest.hpp"

namespace
{
    void parseHeaders(Client &client);
    string_view trimWhiteSpace(string_view sv);
}

bool HttpRequest::parseHttpHeader(Client &client, const char *buff, size_t receivedBytes)
{
    client._header.append(buff, receivedBytes);

    if (client._header.size() > RunServers::getRamBufferLimit())
        throw ErrorCodeClientException(client, 413, "Header size sent by client larger then ramBufferLimit");

    size_t headerEnd = client._header.find("\r\n\r\n");
    if (headerEnd == string::npos)
        return false;

    client._body = client._header.substr(headerEnd + CRLF2_LEN);
    client._header = client._header.erase(headerEnd + CRLF2_LEN);

    if (client._header.find('\0') != string::npos)
        throw ErrorCodeClientException(client, 400, "Null terminator found in header request at index: " + to_string(client._header.find('\0')));

    parseHeaders(client);
    validateHEAD(client);
    
    // Handle Connection header properly
    auto connectionHeader = client._headerFields.find("Connection");
    if (connectionHeader != client._headerFields.end()) {
        string_view connValue = connectionHeader->second;
        if (connValue == "close") {
            client._keepAlive = false;
        } else if (connValue == "keep-alive") {
            client._keepAlive = true;
        } else {
            throw ErrorCodeClientException(client, 400, "Invalid Connection header value: " + string(connValue));
        }
    }
    
    // Check if there is a null character in buff
    if (client._header.find('\0') != string::npos)
       throw ErrorCodeClientException(client, 400, "Null bytes not allowed in HTTP request");

    if (client._method == "POST")
    {
        getContentType(client); // TODO return isn't used at all
        auto transferEncodingHeader = client._headerFields.find("Transfer-Encoding");
        if (transferEncodingHeader != client._headerFields.end() && transferEncodingHeader->second == "chunked")
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

namespace
{
    void parseHeaders(Client &client)
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

    string_view trimWhiteSpace(string_view sv)
    {
        const char *ws = " \t";
        size_t first = sv.find_first_not_of(ws);
        if (first == string_view::npos)
            return {}; // all whitespace
        size_t last = sv.find_last_not_of(ws);
        return sv.substr(first, last - first + 1);
    }
}