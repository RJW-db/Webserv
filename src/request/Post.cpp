#include <HttpRequest.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>


// bool HttpRequest::processHttpBody(Client &client)
// {
//     string content;
//     size_t totalWriteSize;
//     getInfoPost(client, content, totalWriteSize);
//     client._rootPath = client._rootPath + "/" + string(client._filename); // here to append filename for post
//     int fd = open(client._rootPath.data(), O_WRONLY | O_TRUNC | O_CREAT, 0700);
//     if (fd == -1)
//     {
//         if (errno == EACCES)
//             throw ErrorCodeClientException(client, 403, "access not permitted for post on file: " + client._rootPath);
//         else
//             throw ErrorCodeClientException(client, 500, "couldn't open file because: " + string(strerror(errno)) + ", on file: " + client._rootPath);
//     }
//     FileDescriptor::setFD(fd);
//     size_t writeSize = (content.size() < totalWriteSize) ? content.size() : totalWriteSize;
//     ssize_t bytesWritten = write(fd, content.data(), writeSize);
//     if (bytesWritten == -1)
//         HandleTransfer::errorPostTransfer(client, 500, "write failed post request: " + string(strerror(errno)), fd);
//     unique_ptr<HandleTransfer> handle;
//     if (bytesWritten == totalWriteSize)
//     {
//         string boundaryCheck = content.substr(bytesWritten);
//         if (HandleTransfer::foundBoundaryPost(client, boundaryCheck, fd) == true)
//         {
//             RunServers::clientHttpCleanup(client);
//             if (client._keepAlive == false)
//                 RunServers::cleanupClient(client);
//             return false;
//         }
//         handle = make_unique<HandleTransfer>(client, fd, static_cast<size_t>(bytesWritten), totalWriteSize, content.substr(bytesWritten));
//     }
//     else
//         handle = make_unique<HandleTransfer>(client, fd, static_cast<size_t>(bytesWritten), totalWriteSize, "");
//     RunServers::insertHandleTransfer(move(handle));
//     return true;
// }

bool HttpRequest::processHttpBody(Client &client)
{
    size_t totalWriteSize;
    string content = client._body.substr(client._bodyEnd + 4);
    getInfoPost(client, content, totalWriteSize);
    client._rootPath = client._rootPath + "/" + string(client._filename); // here to append filename for post
    int fd = open(client._rootPath.data(), O_WRONLY | O_TRUNC | O_CREAT, 0700);
    bool hasBoundaryPrefix = false;
    if (fd == -1)
    {
        if (errno == EACCES)
            throw ErrorCodeClientException(client, 403, "access not permitted for post on file: " + client._rootPath);
        else
            throw ErrorCodeClientException(client, 500, "couldn't open file because: " + string(strerror(errno)) + ", on file: " + client._rootPath);
    }
    FileDescriptor::setFD(fd);
    // size_t boundaryBufferSize = client._bodyBoundary.size() + 6; // \r\n\r\n--boundary
    // size_t writeSize = (boundaryBufferSize >= content.size()) ? 0 : content.size() - boundaryBufferSize;
    // ssize_t boundaryFound = content.find(client._bodyBoundary);
    // if (boundaryFound != string::npos)
    // {
    //     for (ssize_t i = boundaryFound; i >= 0; i--)
    //     {
    //         if (content[i] == '\n')
    //         {
    //             if (i > 0 && content[i - 1] == '\r')
    //             {
    //                 if (i > 1 && content[i - 1] != '\n')
    //                 {
    //                     writeSize = i - 1; // TODO underflow 0 -1
    //                     break ;
    //                 }
    //             }
    //         }
    //     }
    // }
    // ssize_t bytesWritten = 0;
    // if (writeSize > 0)
    // {
    //     ssize_t bytesWritten = write(fd, content.data(), writeSize);
    //     if (bytesWritten == -1)
    //         HandleTransfer::errorPostTransfer(client, 500, "write failed post request: " + string(strerror(errno)), fd);
    // }
    // if (boundaryFound != string::npos)
    // {
    //         FileDescriptor::closeFD(fd);
    //         string body = client._rootPath + '\n';
    //         string headers =
    //             "HTTP/1.1 200 OK\r\n"
    //             "Content-Type: text/plain\r\n"
    //             "Connection: " +
    //             string(client._keepAlive ? "keep-alive" : "close") + "\r\n"
    //             "Content-Length: " +
    //             to_string(body.size()) + "\r\n"
    //                                      "\r\n" +
    //             body;

    //         send(client._fd, headers.data(), headers.size(), 0);
    //         RunServers::setEpollEvents(client._fd, EPOLL_CTL_MOD, EPOLLIN);
    //         RunServers::logMessage(5, "POST success, clientFD: ", client._fd, ", rootpath: ", client._rootPath);
    //         RunServers::clientHttpCleanup(client);
    //         return false;
    // }
    unique_ptr<HandleTransfer> handle;
    handle = make_unique<HandleTransfer>(client, fd, client._body.size(), content.substr(0));
    ssize_t bytesWritten = 0;
    size_t boundaryFound = handle->FindBoundaryAndWrite(bytesWritten);
    if (boundaryFound != string::npos)
    {
        size_t offset = 0;
        handle->_fileBuffer = handle->_fileBuffer.substr(client._bodyBoundary.size() + boundaryFound - bytesWritten);
        RunServers::setEpollEvents(client._fd, EPOLL_CTL_MOD, EPOLLIN);
        RunServers::logMessage(5, "POST success, clientFD: ", client._fd, ", rootpath: ", client._rootPath);
        handle->_foundEndingBoundary = true;
        if (handle->validateFinalCRLF() == true)
        {
            if (client._keepAlive == false)
                RunServers::cleanupClient(client);
            else
                RunServers::clientHttpCleanup(client);
            return false;
        }
    }
    RunServers::insertHandleTransfer(move(handle));
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

void HttpRequest::getInfoPost(Client &client, string &content, size_t &totalWriteSize)
{
    HttpRequest::getContentLength(client);
    HttpRequest::getBodyInfo(client);
    HttpRequest::getContentType(client); // TODO return isn't used at all
    size_t headerOverhead = client._bodyEnd + 4;                       // \r\n\r\n
    size_t boundaryOverhead = client._bodyBoundary.size() + 8; // --boundary-- + \r\n\r\n
    totalWriteSize = client._contentLength - headerOverhead - boundaryOverhead;
}

void HttpRequest::getContentLength(Client &client)
{
    auto contentLength = client._headerFields.find("Content-Length");
    if (contentLength == client._headerFields.end())
        throw RunServers::ClientException("Broken POST request");
    const string_view content = contentLength->second;
    if (content.empty())
    {
        throw RunServers::LengthRequiredException("Content-Length header is empty.");
    }
    for (size_t i = 0; i < content.size(); ++i)
    {
        if (!isdigit(static_cast<unsigned char>(content[i])))
            throw RunServers::ClientException("Content-Length contains non-digit characters.");
    }
    long long value;
    try
    {
        value = stoll(content.data());
        if (value < 0)
            throw RunServers::ClientException("Content-Length cannot be negative.");

        if (static_cast<size_t>(value) > client._location.getClientBodySize())
            throw ErrorCodeClientException(client, 413, "Content-Length exceeds maximum allowed:" + to_string(value)); // (413, "Payload Too Large");

        if (value == 0)
            throw RunServers::ClientException("Content-Length cannot be zero.");
    }
    catch (const invalid_argument &)
    {
        throw RunServers::ClientException("Content-Length is invalid (not a number).");
    }
    catch (const out_of_range &)
    {
        throw RunServers::ClientException("Content-Length value is out of range.");
    }
    client._contentLength = static_cast<size_t>(value);
}