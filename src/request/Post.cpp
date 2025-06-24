#include <HttpRequest.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>

void getInfoPost(Client &client, string &content, size_t &totalWriteSize, size_t bodyEnd)
{
    HttpRequest::getContentLength(client);
    HttpRequest::getBodyInfo(client);
    HttpRequest::getContentType(client);
    content = client._body.substr(bodyEnd + 4);
    size_t headerOverhead = bodyEnd + 4;                       // \r\n\r\n
    size_t boundaryOverhead = client._bodyBoundary.size() + 8; // --boundary-- + \r\n\r\n
    totalWriteSize = client._contentLength - headerOverhead - boundaryOverhead;
}

void HttpRequest::POST(Client &client)
{
    string content;
    size_t totalWriteSize;
    size_t bodyEnd = client._body.find("\r\n\r\n");
    if (bodyEnd == string::npos)
        return;
    getInfoPost(client, content, totalWriteSize, bodyEnd);
    string filename = client._location.getPath() + '/' + string(client._filename);
    int fd = open(filename.data(), O_WRONLY | O_TRUNC | O_CREAT, 0700);
    if (fd == -1)
    {
        if (errno == EACCES)
            throw ErrorCodeClientException(client, 403, "access not permitted for post on file: " + filename);
        else
            throw ErrorCodeClientException(client, 500, "couldn't open file because: " + string(strerror(errno)) + ", on file: " + filename);
    }
    FileDescriptor::setFD(fd);
    size_t writeSize = (content.size() < totalWriteSize) ? content.size() : totalWriteSize;
    ssize_t bytesWritten = write(fd, content.data(), writeSize);
    if (bytesWritten == -1)
    {
        FileDescriptor::closeFD(fd);
        // TODO remove(filename.data());
        throw ErrorCodeClientException(client, 500, "write failed post request: " + string(strerror(errno)));
    }
    unique_ptr<HandleTransfer> handle;
    if (bytesWritten == totalWriteSize)
    {
        if (content.find("--" + string(client._bodyBoundary) + "--\r\n") == totalWriteSize + 2)
        {
            FileDescriptor::closeFD(fd);
            // Fix: Complete HTTP response with proper headers
            string ok = HttpRequest::HttpResponse(200, "", 0);
            send(client._fd, ok.data(), ok.size(), 0);
            return;
        }
        handle = make_unique<HandleTransfer>(client, fd, static_cast<size_t>(bytesWritten), totalWriteSize, content.substr(bytesWritten));
    }
    else
        handle = make_unique<HandleTransfer>(client, fd, static_cast<size_t>(bytesWritten), totalWriteSize, "");
    RunServers::insertHandleTransfer(move(handle));
}

ContentType HttpRequest::getContentType(Client &client)
{
    auto it = client._headerFields.find("Content-Type");
    if (it == client._headerFields.end())
        throw RunServers::ClientException("Missing Content-Type");
    const string_view ct = it->second;
    if (ct == "application/x-www-form-urlencoded")
    {
        client._contentType = ct;
        return FORM_URLENCODED;
    }
    if (ct == "application/json")
    {
        client._contentType = ct;
        return JSON;
    }
    if (ct == "text/plain")
    {
        client._contentType = ct;
        return TEXT;
    }
    if (ct.find("multipart/form-data") == 0)
    {
        size_t semi = ct.find(';');
        if (semi != std::string_view::npos)
        {
            client._contentType = ct.substr(0, semi);
            size_t boundaryPos = ct.find("boundary=", semi);
            if (boundaryPos != std::string_view::npos)
                client._bodyBoundary = ct.substr(boundaryPos + 9); // 9 = strlen("boundary=")
            else
                throw RunServers::ClientException("Malformed multipart Content-Type: boundary not found");
        }
        else
            throw RunServers::ClientException("Malformed HTTP header line: " + string(ct));
        return MULTIPART;
    }
    return UNSUPPORTED;
}

void HttpRequest::getBodyInfo(Client &client)
{
    size_t cdPos = client._body.find("Content-Disposition:");
    if (cdPos == string::npos)
        throw RunServers::ClientException("Content-Disposition header not found in multipart body");

    // Extract the Content-Disposition line
    size_t cdEnd = client._body.find("\r\n", cdPos);
    string_view cdLine = string_view(client._body).substr(cdPos, cdEnd - cdPos);

    string filenameKey = "filename=\"";
    size_t fnPos = cdLine.find(filenameKey);
    if (fnPos != string::npos)
    {
        size_t fnStart = fnPos + filenameKey.size();
        size_t fnEnd = cdLine.find("\"", fnStart);
        client._filename = cdLine.substr(fnStart, fnEnd - fnStart);
        if (client._filename.empty())
            throw RunServers::ClientException("Filename is empty in Content-Disposition header");
    }
    else
        throw RunServers::ClientException("Filename not found in Content-Disposition header");

    const string contentType = "Content-Type: ";
    size_t position = client._body.find(contentType);

    if (position == string::npos)
        throw RunServers::ClientException("Content-Type header not found in multipart/form-data body part");

    size_t fileStart = client._body.find("\r\n\r\n", position) + 4;
    // size_t fileEnd = client._body.find("\r\n--" + std::string(client._bodyBoundary) /* + "--\r\n" */, fileStart);

    // if (position == string::npos)
    //     throw RunServers::ClientException("Malformed or missing Content-Type header in multipart/form-data body part");

    // client._fileContent = string_view(client._body).substr(fileStart, fileEnd - fileStart);
}
