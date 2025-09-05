#include "ErrorCodeClientException.hpp"
#include "HttpRequest.hpp"
#include "RunServer.hpp"
#include "Constants.hpp"
#include "Logger.hpp"

string HttpRequest::HttpResponse(const Client &client, uint16_t code, const string &path, size_t fileSize)
{
    static const map<uint16_t, string> responseCodes = {
        {200, "OK"},
        {201, "Created"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {303, "See Other"},
        {307, "Temporary Redirect"},
        {308, "Permanent Redirect"},
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
        {504, "Gateway Timeout"}
    };

    map<uint16_t, string>::const_iterator it = responseCodes.find(code);
    if (it == responseCodes.end())
        throw runtime_error("Couldn't find code");

    ostringstream response;
    response << "HTTP/1.1 " << to_string(it->first) << ' ' << it->second << CRLF;
    if (!path.empty())
        response << "Content-Type: " << getMimeType(path) << CRLF;
    response << "Content-Length: " << fileSize << CRLF;
    response << "Connection: " + string(client._keepAlive ? "keep-alive" : "close") + CRLF;
    response << CRLF;
    return response.str();
}

string HttpRequest::getMimeType(const string &path)
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

string HttpRequest::createResponseCgi(Client &client, const string &input)
{
    size_t headerSize = input.find(CRLF2);
    bool hasBody = (input.size() > headerSize + CRLF2_LEN);
    map<string_view, string_view> headerFields;
    parseCgiHeaders(input, headerFields, headerSize);
    validateCgiHeaders(client, headerFields, hasBody);
    return buildCgiResponse(client, headerFields, input, headerSize, hasBody);
}

map<string_view, string_view> HttpRequest::parseCgiHeaders(const string &input, map<string_view, string_view> &headerFields, size_t headerSize)
{
    size_t pos = 0;
    while (pos < headerSize)
    {
        size_t end = input.find(CRLF, pos);
        if (end == string::npos) 
            break;
        string_view line(&input[pos], end - pos);
        size_t colon = line.find(':');
        if (colon != string_view::npos) {
            string_view key = line.substr(0, colon);
            string_view value = line.substr(colon + 1);
            size_t firstNonSpace = value.find_first_not_of(" ");
            if (firstNonSpace != string_view::npos)
                value = value.substr(firstNonSpace);
            headerFields[key] = value;
        }
        pos = end + CRLF_LEN;
    }
    return headerFields;
}

void HttpRequest::validateCgiHeaders(Client &client, const map<string_view, string_view> &headerFields, bool hasBody)
{
    if (headerFields.find("Status") == headerFields.end() || headerFields.at("Status").empty())
        throw ErrorCodeClientException(client, 500, "invalid response from cgi process with missing header Status");
    
    if (hasBody && headerFields.count("Content-Type") < 1)
        throw ErrorCodeClientException(client, 500, "invalid response from cgi process with missing header Content-Type");
}

string HttpRequest::buildCgiResponse(const Client &client, const map<string_view, string_view> &headerFields, 
                                const string &input, size_t headerSize, bool hasBody)
{
    ostringstream response;
    response << "HTTP/1.1 " << headerFields.at("Status") << CRLF;
    response << "Connection: " + string(client._keepAlive ? "keep-alive" : "close") + CRLF;
    
    if (hasBody)
    {
        response << "Content-Type: " << headerFields.at("Content-Type") << CRLF;
        if (headerFields.count("Content-Length") > 0)
            response << "Content-Length: " << headerFields.at("Content-Length") << CRLF;
        else
            response << "Content-Length: " << input.size() - headerSize - CRLF2_LEN << CRLF;
        response << CRLF;
        response << input.substr(headerSize + CRLF2_LEN);
    }
    else
        response << CRLF;
    
    return response.str();
}
