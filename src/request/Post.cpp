#include <HttpRequest.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HandleTransfer.hpp>

bool HttpRequest::processHttpBody(Client &client)
{
    size_t totalWriteSize;
    string content = client._body.substr(client._bodyEnd + 4);
    getInfoPost(client, content, totalWriteSize);
    client._filenamePath = client._rootPath + "/" + string(client._filename); // here to append filename for post
    int fd = open(client._filenamePath.data(), O_WRONLY | O_TRUNC | O_CREAT, 0700);
    if (fd == -1)
    {
        if (errno == EACCES)
            throw ErrorCodeClientException(client, 403, "access not permitted for post on file: " + client._filenamePath);
        else
            throw ErrorCodeClientException(client, 500, "couldn't open file because: " + string(strerror(errno)) + ", on file: " + client._filenamePath);
    }
    FileDescriptor::setFD(fd);
    unique_ptr<HandleTransfer> handle;
    handle = make_unique<HandleTransfer>(client, fd, client._body.size(), content);
    if (handle->handlePostTransfer(false) == true)
    {
        if (client._keepAlive == false)
            RunServers::cleanupClient(client);
        else
        {
            RunServers::clientHttpCleanup(client);
        }
        return false;
    }
    RunServers::insertHandleTransfer(move(handle));
    return true;
}


bool HttpRequest::processHttpChunkBody(Client &client, int targetFilePathFD)
{
    size_t totalWriteSize;
    HttpRequest::getBodyInfo(client);
    client._filenamePath = client._rootPath + "/" + string(client._filename); // here to append filename for post
    int fd = open(client._filenamePath.data(), O_WRONLY | O_TRUNC | O_CREAT, 0700);
    if (fd == -1)
    {
        if (errno == EACCES)
            throw ErrorCodeClientException(client, 403, "access not permitted for post on file: " + client._filenamePath);
        else
            throw ErrorCodeClientException(client, 500, "couldn't open file because: " + string(strerror(errno)) + ", on file: " + client._filenamePath);
    }
    FileDescriptor::setFD(fd);
    targetFilePathFD = fd;
    return true;
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
        if (semi != string_view::npos)
        {
            client._contentType = ct.substr(0, semi);
            size_t boundaryPos = ct.find("boundary=", semi);
            if (boundaryPos != string_view::npos)
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
    {
        throw RunServers::ClientException("Filename not found in Content-Disposition header");
    }

    const string contentType = "Content-Type: ";
    size_t position = client._body.find(contentType);

    if (position == string::npos)
        throw RunServers::ClientException("Content-Type header not found in multipart/form-data body part");

    size_t fileStart = client._body.find("\r\n\r\n", position) + 4;
    // size_t fileEnd = client._body.find("\r\n--" + string(client._bodyBoundary) /* + "--\r\n" */, fileStart);

    // if (position == string::npos)
    //     throw RunServers::ClientException("Malformed or missing Content-Type header in multipart/form-data body part");

    // client._fileContent = string_view(client._body).substr(fileStart, fileEnd - fileStart);
}
