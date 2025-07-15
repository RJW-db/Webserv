#include <HttpRequest.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>

bool HttpRequest::processHttpBody(Client &client)
{
    size_t totalWriteSize;
    std::cout << "client.body: " << client._body << ", bodyend: " << client._bodyEnd << std::endl; //testcout
    string content = client._body.substr(client._bodyEnd + 4);
    getInfoPost(client, content, totalWriteSize);
    client._filenamePath = client._rootPath + "/" + string(client._filename); // here to append filename for post
    int fd = open(client._filenamePath.data(), O_WRONLY | O_TRUNC | O_CREAT, 0700);
    bool hasBoundaryPrefix = false;
    if (fd == -1)
    {
        if (errno == EACCES)
            throw ErrorCodeClientException(client, 403, "access not permitted for post on file: " + client._filenamePath);
        else
            throw ErrorCodeClientException(client, 500, "couldn't open file because: " + string(strerror(errno)) + ", on file: " + client._filenamePath);
    }
    FileDescriptor::setFD(fd);
    unique_ptr<HandleTransfer> handle;
    handle = make_unique<HandleTransfer>(client, fd, client._body.size(), content.substr(0));
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




void HttpRequest::validateChunkSizeLine(const string &input)
{
    if (input.size() < 1)
    {
        // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
    }

    // size_t hexPart = input.size() - 2;
    if (all_of(input.begin(), input.end(), ::isxdigit) == false)
    {
        // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
    }
        
    // if (input[hexPart] != '\r' && input[hexPart + 1] != '\n')
    // {
    //     // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
    // }
}
#include <string>
uint64_t HttpRequest::parseChunkSize(const string &input)
{
    uint64_t chunkTargetSize;
    stringstream ss;
    ss << hex << input;
    ss >> chunkTargetSize;
    // cout << "chunkTargetSize " << chunkTargetSize << endl;
    // if (static_cast<size_t>(chunkTargetSize) > client._location.getClientBodySize())
    // {
    //     throw ErrorCodeClientException(client, 413, "Content-Length exceeds maximum allowed: " + to_string(chunkTargetSize)); // (413, "Payload Too Large");
    // }
    return chunkTargetSize;
}

void HttpRequest::ParseChunkStr(const string &input, uint64_t chunkTargetSize)
{
    if (input.size() - 2 != chunkTargetSize)
    {
        // throw ErrorCodeClientException(client, 400, "Chunk data does not match declared chunk size");
    }

    if (input[chunkTargetSize] != '\r' || input[chunkTargetSize + 1] != '\n')
    {
        // throw ErrorCodeClientException(client, 400, "chunk data missing CRLF");
    }
}

void unchunkingProcess(Client &client)
{

}
void HttpRequest::handleChunks(Client &client)
{
    if (client._chunkTargetSize == 0)
    {
        size_t crlf = client._body.find("\r\n", client._bodyPos);
        if (crlf == string::npos)
        {
            return;
        }
        string chunkSizeLine = client._body.substr(client._bodyPos, crlf);
        validateChunkSizeLine(chunkSizeLine);
        
        client._chunkTargetSize = parseChunkSize(chunkSizeLine);
        client._chunkBodyPos = crlf + 2;
        
    }

    // if (client._body.size() >= client._chunkTargetSize + client._chunkBodyPos &&
    //     client._body[client._chunkTargetSize + client._chunkBodyPos] == '\r' &&
    //     client._body[client._chunkTargetSize + client._chunkBodyPos + 1] == '\n')
    if (client._body.size() - client._bodyPos >= client._chunkTargetSize + client._chunkBodyPos &&
        client._body[client._bodyPos + client._chunkTargetSize + client._chunkBodyPos] == '\r' &&
        client._body[client._bodyPos + client._chunkTargetSize + client._chunkBodyPos + 1] == '\n')
    {
        string_view boundaryCheck(&client._body[client._bodyPos + client._chunkBodyPos + 2], client._bodyBoundary.size());
        if (client._body.substr(client._bodyPos + client._chunkBodyPos, 2) != "--" ||
            boundaryCheck != client._bodyBoundary)
        {
            ErrorCodeClientException(client, 400, "Malformed boundary");
        }
        size_t endBodyHeader = client._body.find("\r\n\r\n", client._bodyPos + client._chunkBodyPos + client._bodyBoundary.size() + 2);
        if (endBodyHeader == string::npos)
        {
            ErrorCodeClientException(client, 400, "body header didn't end in \\r\\n\\r\\n");
        }
        client._bodyHeader = client._body.substr(client._bodyPos + client._chunkBodyPos, endBodyHeader + 4 - client._chunkBodyPos);

        client._unchunkedBody = client._body.substr(client._bodyPos + endBodyHeader + 4);
        exit(0);
    }
}
