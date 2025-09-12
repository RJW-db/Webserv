#include <string_view>
#include <algorithm>
#include <cassert>
#include "ErrorCodeClientException.hpp"
#include "HandleTransfer.hpp"
#include "HttpRequest.hpp"
#include "RunServer.hpp"
#include "Constants.hpp"
#include "utils.hpp"

#include "Logger.hpp"
namespace
{
    void processThemeForm(Client &client);
    string extractContentDispositionLine(Client &client, const string &buff);
    string extractFilenameFromContentDisposition(Client &client, const string &cdLine);
    void validateMultipartContentType(Client &client, const string &buff, const string &filename);
}

void HttpRequest::POST(Client &client)
{
    if (client._contentType == FORM_URLENCODED && client._isCgi == false) {
        processThemeForm(client);
        return;
    }

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
    try {
        uint64_t value = stoullSafe(contentLength->second);
        if (static_cast<size_t>(value) > client._location.getClientMaxBodySize())
            throw ErrorCodeClientException(client, 413, "Content-Length exceeds maximum allowed: " + to_string(value));
        if (value == 0)
            throw ErrorCodeClientException(client, 400, "Content-Length cannot be zero.");

        client._contentLength = static_cast<size_t>(value);
    }
    catch (const runtime_error &e) {
        throw ErrorCodeClientException(client, 400, "Content-Length header is invalid: " + string(e.what()));
    }
}

void HttpRequest::getContentType(Client &client)
{
    auto it = client._headerFields.find("Content-Type");
    if (it == client._headerFields.end())
        throw ErrorCodeClientException(client, 400, "Content-Type header not found in request");
    const string_view ct(it->second);
    if (ct.find(ContentTypeString[MULTIPART_FORM_DATA]) == 0) { // "multipart/form-data"
        size_t semi = ct.find(';');
        if (semi != string_view::npos) {
            size_t boundaryPos = ct.find("boundary=", semi);
            if (boundaryPos != string_view::npos)
                client._boundary = ct.substr(boundaryPos + 9); // 9 = strlen("boundary=")
            else
                throw ErrorCodeClientException(client, 400, "Boundary not found in Content-Type header");
        }
        else
            throw ErrorCodeClientException(client, 400, "Malformed HTTP header line: " + string(ct));
        client._contentType = MULTIPART_FORM_DATA;
    }
    else if (ct.find(ContentTypeString[FORM_URLENCODED]) == 0) { // "application/x-www-form-urlencoded"
        client._contentType = FORM_URLENCODED;
        if (client._requestPath != "/set-theme" && client._isCgi == false)
            throw ErrorCodeClientException(client, 415, "Unsupported Content-Type for this endpoint: " + string(ct));
    }
    else {
        client._contentType = UNSUPPORTED;
        throw ErrorCodeClientException(client, 400, "Unsupported Content-Type: " + string(ct));
    }
}

void HttpRequest::getBodyInfo(Client &client, const string &buff)
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
    generateFilenameUuid(uuid);

    size_t totalLength = filename.size() + UUID_SIZE;
    if (totalLength > NAME_MAX) {
        size_t leftOvers = totalLength - NAME_MAX;
        if (filename.size() > leftOvers)
            filename.erase(filename.size() - leftOvers);
        else
            throw ErrorCodeClientException(client, 413, "Filename too long to accommodate unique identifier");
    }

    size_t lastDotIndex = filename.find_last_of('.');
    if (lastDotIndex != string::npos && lastDotIndex > 0)
        filename.insert(lastDotIndex, uuid);
    else
        filename += uuid;

    client._filenamePath = client._location.getUploadStore() + '/' + client._filename; // here to append filename for post
    if (client._filenamePath.size() > PATH_MAX)
        throw ErrorCodeClientException(client, 413, "Full path to create file is too long");
}

namespace
{

    void processThemeForm(Client &client)
    {
        replace(client._body.begin(), client._body.end(), '+', ' ');
        SessionData &sessionData = RunServers::getSessionData(client._sessionId);

        if (client._body.empty() || client._body.size() < 6)
            throw ErrorCodeClientException(client, 400, "Body too short or empty for form data");

        if (client._body.substr(0, 6) != "theme=")
            throw ErrorCodeClientException(client, 400, "Unsupported form data key: " + client._body);

        string_view theme(client._body.data() + 6, client._body.size() - 6);
        if (theme == "light")
            sessionData.darkMode = false;
        else if (theme == "dark")
            sessionData.darkMode = true;
        else
            throw ErrorCodeClientException(client, 400, "Unsupported theme value: " + string(theme));

        string responseStr = HttpRequest::HttpResponse(client, 200, "", 0);
        unique_ptr<HandleTransfer> handle = make_unique<HandleToClientTransfer>(client, responseStr);
        RunServers::insertHandleTransfer(move(handle));
    }

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

        if (filename.find('/') != string::npos || filename.find('\\') != string::npos)
            throw ErrorCodeClientException(client, 400, "Directory separators not allowed in filename: " + filename);
        
        if (filename == ".." || filename == "." || 
            filename.find("../") != string::npos || filename.find("..\\") != string::npos ||
            filename.find("/..") != string::npos || filename.find("\\..") != string::npos)
            throw ErrorCodeClientException(client, 400, "Path traversal detected in filename: " + filename);

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
