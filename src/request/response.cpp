#include "HttpRequest.hpp"




string HttpRequest::HttpResponse(Client &client, uint16_t code, string path, size_t fileSize)
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

string HttpRequest::createResponseCgi(Client &client, string &input)
{
    Logger::log(DEBUG, "input received:", input); //testlog
    size_t headerSize = input.find(CRLF2);

    map<string_view, string_view> headerFields;
    size_t pos = 0;
    while (pos < headerSize)
    {
        size_t end = input.find(CRLF, pos);
        if (end == string::npos) 
            break;
        string_view line(input.data() + pos, end - pos);
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
    ostringstream response;
    // if (headerFields["Status"].empty())
    //     throw ErrorCodeClientException(client, 500, "invalid response from cgi process with missing header Status");
    response << "HTTP/1.1 " << headerFields["Status"] << "\r\n";
    response << "Connection: " + string(client._keepAlive ? "keep-alive" : "close") + "\r\n";
    if (input.size() > headerSize + CRLF2_LEN)
    {
        // if (headerFields.count("Content-Type") < 1)
        //     throw ErrorCodeClientException(client, 500, "invalid response from cgi process with missing header Content-Type");
        response << "Content-Type: " << headerFields["Content-Type"] << "\r\n";
        if (headerFields.count("Content-Length") > 0)
            response << "Content-Length: " << headerFields["Content-Length"] << "\r\n";
        else
        {
            Logger::log(DEBUG, "content length set by server"); //testlog
            response << "Content-Length: " << input.size() - headerSize - CRLF2_LEN << "\r\n";
        }
            
        response << "\r\n";
        response << input.substr(headerSize + CRLF2_LEN);
    }
    else
        response << "\r\n";
    return response.str();
}