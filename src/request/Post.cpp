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
    string content;
    size_t totalWriteSize;
    std::cout << escape_special_chars(client._body) << std::endl << endl; //testcout
    getInfoPost(client, content, totalWriteSize);
    client._rootPath = client._rootPath + "/" + string(client._filename); // here to append filename for post
    int fd = open(client._rootPath.data(), O_WRONLY | O_TRUNC | O_CREAT, 0700);
    if (fd == -1)
    {
        if (errno == EACCES)
            throw ErrorCodeClientException(client, 403, "access not permitted for post on file: " + client._rootPath);
        else
        {
            std::cout << "method: " << client._method << std::endl; //testcout
            throw ErrorCodeClientException(client, 500, "couldn't open file because: " + string(strerror(errno)) + ", on file: " + client._rootPath);
        }
    }
    FileDescriptor::setFD(fd);
    // if (client._body.find(client._bodyBoundary) != 0)
         
    size_t boundaryBufferSize = client._bodyBoundary.size() + 6; // \r\n\r\n--boundary
    size_t writeSize = (boundaryBufferSize >= content.size()) ? 0 : content.size() - boundaryBufferSize;
    ssize_t boundaryFound = content.find(client._bodyBoundary);
    if (boundaryFound != string::npos)
    {
        for (ssize_t i = boundaryFound; i >= 0; i--)
        {
            if (content[i] == '\n')
            {
                if (i > 0 && content[i - 1] == '\r')
                {
                    if (i > 1 && content[i - 1] != '\n')
                    {
                        writeSize = i - 1; // TODO underflow 0 -1
                        break ;
                    }
                }
            }
        }
    }
    ssize_t bytesWritten = 0;
    if (writeSize > 0)
    {
        ssize_t bytesWritten = write(fd, content.data(), writeSize);
        if (bytesWritten == -1)
            HandleTransfer::errorPostTransfer(client, 500, "write failed post request: " + string(strerror(errno)), fd);
    }
    if (boundaryFound != string::npos)
    {
            FileDescriptor::closeFD(fd);
            string body = client._rootPath + '\n';
            string headers =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: " +
                string(client._keepAlive ? "keep-alive" : "close") + "\r\n"
                "Content-Length: " +
                to_string(body.size()) + "\r\n"
                                         "\r\n" +
                body;

            send(client._fd, headers.data(), headers.size(), 0);
            RunServers::setEpollEvents(client._fd, EPOLL_CTL_MOD, EPOLLIN);
            RunServers::logMessage(5, "POST success, clientFD: ", client._fd, ", rootpath: ", client._rootPath);
            RunServers::clientHttpCleanup(client);
            return false;
    }
    unique_ptr<HandleTransfer> handle;
    // if (bytesWritten == totalWriteSize)
    // {
    // string boundaryCheck = content.substr(bytesWritten);
    // if (HandleTransfer::foundBoundaryPost(client, boundaryCheck, fd) == true)
    // {
    //     RunServers::clientHttpCleanup(client);
    //     if (client._keepAlive == false)
    //         RunServers::cleanupClient(client);
    //     return false;
    // }

    handle = make_unique<HandleTransfer>(client, fd, static_cast<size_t>(bytesWritten), totalWriteSize, content.substr(bytesWritten));
    // }
    // else
        // handle = make_unique<HandleTransfer>(client, fd, static_cast<size_t>(bytesWritten), totalWriteSize, "");
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
    // std::cout << "body: " << client._body.substr(0, 100) << std::endl; //testcout
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
    // size_t fileEnd = client._body.find("\r\n--" + string(client._bodyBoundary) /* + "--\r\n" */, fileStart);

    // if (position == string::npos)
    //     throw RunServers::ClientException("Malformed or missing Content-Type header in multipart/form-data body part");

    // client._fileContent = string_view(client._body).substr(fileStart, fileEnd - fileStart);
}




void HttpRequest::validateChunkSizeLine(const string &input)
{
    if (input.size() < 1)
    {
        cout << "wrong1" << endl; //testcout
        // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
    }

    // size_t hexPart = input.size() - 2;
    if (all_of(input.begin(), input.end(), ::isxdigit) == false)
    {
        cout << "wrong2" << endl; //testcout
        // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
    }
        
    // if (input[hexPart] != '\r' && input[hexPart + 1] != '\n')
    // {
    //     cout << "wrong3" << endl; //testcout
    //     if (input[hexPart] == '\r')
    //         cout << "\\r" << endl; //testcout
    //     if (input[hexPart + 1] == '\n')
    //         cout << "\\n" << endl; //testcout
    //     // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
    // }
}
#include <string>
uint64_t HttpRequest::parseChunkSize(const string &input)
{
    uint64_t chunkSize;
    stringstream ss;
    ss << hex << input;
    ss >> chunkSize;
    // cout << "chunkSize " << chunkSize << endl;
    // if (static_cast<size_t>(chunkSize) > client._location.getClientBodySize())
    // {
    //     throw ErrorCodeClientException(client, 413, "Content-Length exceeds maximum allowed: " + to_string(chunkSize)); // (413, "Payload Too Large");
    // }
    return chunkSize;
}

void HttpRequest::ParseChunkStr(const string &input, uint64_t chunkSize)
{
    if (input.size() - 2 != chunkSize)
    {
        cout << "test1" << endl; //testcout
        // throw ErrorCodeClientException(client, 400, "Chunk data does not match declared chunk size");
    }

    if (input[chunkSize] != '\r' || input[chunkSize + 1] != '\n')
    {
        cout << "test2" << endl; //testcout
        // throw ErrorCodeClientException(client, 400, "chunk data missing CRLF");
    }
}


void HttpRequest::handleChunks(Client &client)
{
    string line = client._body.substr(client._chunkPos);
    // std::cout << escape_special_chars(line) <<endl; //testcout
    size_t crlf = line.find("\r\n");
    if (crlf == string::npos)
    {
        std::cout << "handleChunks 1" << std::endl; //testcout
        return;
    }
    std::cout << escape_special_chars(line) <<endl<<endl; //testcout
    string chunkSizeLine = line.substr(0, crlf);
    std::cout << escape_special_chars(chunkSizeLine) << endl; //testcout
    validateChunkSizeLine(chunkSizeLine);
    
    uint64_t chunkSize = parseChunkSize(chunkSizeLine);
    std::cout << "chunkSize = " << chunkSize << endl; //testcout

    size_t chunkDataCrlf = line.find("\r\n", crlf + 2);
    if (chunkDataCrlf == string::npos)
    if (crlf == string::npos)
    {
        std::cout << "handleChunks 2" << std::endl; //testcout
        return; // but should start looking for chunkData and not chunkSize
    }
    string chunkData = line.substr(crlf + 2, chunkDataCrlf);
    std::cout << escape_special_chars(chunkData) << std::endl; //testcout

    std::cout << "chunkData = " << chunkData.size() << std::endl; //testcout

    // if (chunkData[39] == '\r')
    //     std::cout << "made it" << std::endl; //testcout
    // std::cout << chunkData[39] << std::endl; //testcout
    exit(0);
}
