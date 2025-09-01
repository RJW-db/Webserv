#include "ErrorCodeClientException.hpp"
#include "HandleTransfer.hpp"
#include "HttpRequest.hpp"
#include "RunServer.hpp"
#include "Constants.hpp"
#include "utils.hpp"
namespace
{
    string extractContentDispositionLine(Client &client, const string &buff);
    string extractFilenameFromContentDisposition(Client &client, const string &cdLine);
    void validateMultipartContentType(Client &client, const string &buff, const string &filename);
}

void HttpRequest::POST(Client &client)
{
    HttpRequest::getContentLength(client);
    unique_ptr<HandleTransfer> handle;
    handle = make_unique<HandlePostTransfer>(client, client._body.size(), client._body);
    if (handle->postTransfer(false) == false)
        RunServers::insertHandleTransfer(move(handle));
}

void HttpRequest::getContentLength(Client &client)
{
    auto contentLength = client._headerFields.find("Content-Length");
    if (contentLength == client._headerFields.end())
        throw ErrorCodeClientException(client, 400, "Content-Length header not found");
    try
    {
        uint64_t value = stoullSafe(contentLength->second);
        if (static_cast<size_t>(value) > client._location.getClientMaxBodySize())
            throw ErrorCodeClientException(client, 413, "Content-Length exceeds maximum allowed: " + to_string(value));
        if (value == 0)
            throw ErrorCodeClientException(client, 400, "Content-Length cannot be zero.");

        client._contentLength = static_cast<size_t>(value);
    }
    catch (const runtime_error &e)
    {
        throw ErrorCodeClientException(client, 400, "Content-Length header is invalid: " + string(e.what()));
    }
}

void HttpRequest::getContentType(Client &client)
{
    auto it = client._headerFields.find("Content-Type");
    if (it == client._headerFields.end())
        throw ErrorCodeClientException(client, 400, "Content-Type header not found in request");
    const string_view ct(it->second);
    if (ct.find("multipart/form-data") == 0)
    {
        size_t semi = ct.find(';');
        if (semi != string_view::npos)
        {
            client._contentType = ct.substr(0, semi);
            size_t boundaryPos = ct.find("boundary=", semi);
            if (boundaryPos != string_view::npos)
                client._boundary = ct.substr(boundaryPos + 9); // 9 = strlen("boundary=")
            else
                throw ErrorCodeClientException(client, 400, "Boundary not found in Content-Type header");
        }
        else
            throw ErrorCodeClientException(client, 400, "Malformed HTTP header line: " + string(ct));
    }
    else if (ct == "application/x-www-form-urlencoded")
        client._contentType = ct;
    else if (ct == "application/json")
        client._contentType = ct;
    else if (ct == "text/plain")
        client._contentType = ct;
    else
        throw ErrorCodeClientException(client, 400, "Unsupported Content-Type: " + string(ct));
}

void HttpRequest::getBodyInfo(Client &client, const string buff)
{
    string cdLine = extractContentDispositionLine(client, buff);
    client._filename = extractFilenameFromContentDisposition(client, cdLine);
    appendUuidToFilename(client, client._filename);
    validateMultipartContentType(client, buff, client._filename);
    if (client._filenamePath.empty())
        throw ErrorCodeClientException(client, 400, "Filename path is empty after processing Content-Disposition");
}

void    HttpRequest::appendUuidToFilename(Client &client, string &filename)
{
    char uuid[UUID_SIZE];
    generateUuid(uuid);

    size_t totalLength = filename.size() + UUID_SIZE;
    if (totalLength > NAME_MAX)
    {
        size_t leftOvers = totalLength - NAME_MAX;
        if (filename.size() > leftOvers)
            filename = filename.substr(0, filename.size() - leftOvers);
        else
            throw ErrorCodeClientException(client, 413, "Filename too long to accommodate unique identifier");
    }

    size_t lastDotIndex = filename.find_last_of('.');
    if (lastDotIndex != string::npos && lastDotIndex > 0)
        filename.insert(lastDotIndex, uuid);
    else
        filename += uuid;

    client._filenamePath = client._rootPath + "/" + client._filename; // here to append filename for post
    if (client._filenamePath.size() > PATH_MAX)
        throw ErrorCodeClientException(client, 413, "Full path to create file is too long");
}

namespace
{
    string extractContentDispositionLine(Client &client, const string &buff)
    {
        const string contentDisposition = "Content-Disposition:";
        size_t cdPos = buff.find(contentDisposition);
        if (cdPos == string::npos)
            throw ErrorCodeClientException(client, 400, "Content-Disposition header not found in multipart body");

        size_t wsStart = cdPos + contentDisposition.size();
        size_t wsEnd = buff.find_first_not_of(" \t", wsStart);
        if (wsEnd == string::npos)
            throw ErrorCodeClientException(client, 400, "Malformed Content-Disposition header: only whitespace found");

        size_t formDataPos = buff.find("form-data", cdPos);
        if (formDataPos != wsEnd)
            throw ErrorCodeClientException(client, 400, "Content-Disposition is not form-data");

        size_t cdEnd = buff.find(CRLF, cdPos);
        return buff.substr(cdPos, cdEnd - cdPos);
    }

    string extractFilenameFromContentDisposition(Client &client, const string &cdLine)
    {
        const string filenameKey = "filename=\"";
        size_t fnPos = cdLine.find(filenameKey);
        if (fnPos == string::npos)
            throw ErrorCodeClientException(client, 400, "Filename not found in Content-Disposition header");

        size_t fnStart = fnPos + filenameKey.size();
        size_t fnEnd = cdLine.find("\"", fnStart);
        string filename = cdLine.substr(fnStart, fnEnd - fnStart);
        if (filename.empty())
            throw ErrorCodeClientException(client, 400, "Filename is empty in Content-Disposition header");
        return filename;
    }

    void validateMultipartContentType(Client &client, const string &buff, const string &filename)
    {
        const string contentType = "Content-Type: ";
        size_t position = buff.find(contentType);
        if (position == string::npos && !filename.empty())
            throw ErrorCodeClientException(client, 400, "Content-Type header not found in multipart body");
    }
}
