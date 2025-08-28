#include "HttpRequest.hpp"

void HttpRequest::processRequest(Client &client)
{
    if (checkAndRedirect(client) ||
        checkAndRunCgi(client))
        return;

    switch (client._useMethod)
    {
        case METHOD_HEAD:
            processHead(client);
            break;
        case METHOD_GET:
            processGet(client);
            break;
        case METHOD_POST:
            processPost(client);
            break;
        case METHOD_DELETE:
            processDelete(client);
            break;
        default:
            throw ErrorCodeClientException(client, 405, "Method Not Allowed: " + client._method);
    }
}

bool HttpRequest::checkAndRedirect(Client &client)
{
    if (client._location.getReturnRedirect().first > 0)
    {
        redirectRequest(client);
        RunServers::clientHttpCleanup(client);
        return true;
    }
    return false;
}

void HttpRequest::redirectRequest(Client &client)
{
    pair<uint16_t, string> redirectPair = client._location.getReturnRedirect();
    string response = HttpResponse(client, redirectPair.first, "", 0);
    size_t index = response.find_first_of(CRLF);
    string locationHeader = "Location: " + redirectPair.second + CRLF;
    response.insert(index + CRLF_LEN, locationHeader);
    unique_ptr handleClient = make_unique<HandleToClientTransfer>(client, response);
    RunServers::insertHandleTransfer(move(handleClient));
}

bool HttpRequest::checkAndRunCgi(Client &client)
{
    if (client._isCgi && client._useMethod != METHOD_POST)
    {
        handleCgi(client, client._body);
        RunServers::clientHttpCleanup(client);
        return true;
    }
    return false;
}

void HttpRequest::processHead(Client &client)
{
    string response = HttpRequest::HttpResponse(client, 200, "txt", 0);
    send(client._fd, response.data(), response.size(), MSG_NOSIGNAL);
}

void HttpRequest::processGet(Client &client)
{
    if (!client._isAutoIndex)
        GET(client);
    else
    {
        SendAutoIndex(client);
        RunServers::clientHttpCleanup(client);
    }
}

void HttpRequest::GET(Client &client)
{
    // Logger::log(DEBUG, client._rootPath); //testlog
    // Logger::log(DEBUG, client._filenamePath); //testlog
    int fd = open(client._filenamePath.data(), R_OK);
    if (fd == -1)
        throw RunServers::ClientException("open failed on file: " + client._filenamePath + ", because: " + string(strerror(errno)));

    FileDescriptor::setFD(fd);
    size_t fileSize = getFileLength(client._filenamePath);
    string responseStr = HttpResponse(client, 200, client._filenamePath, fileSize);
    auto handle = make_unique<HandleGetTransfer>(client, fd, responseStr, static_cast<size_t>(fileSize));
    RunServers::insertHandleTransfer(move(handle));
}

void HttpRequest::SendAutoIndex(Client &client)
{
    struct dirent *en;
    DIR *dr = opendir(client._rootPath.data());
    string filenames;
    if (dr)
    {
        while ((en = readdir(dr)) != NULL) // TODO not protected
        {
            filenames += "<a href=\"";
            filenames += client._requestPath;
            if (!client._requestPath.empty() && client._requestPath.back() != '/')
                filenames += "/";
            filenames += en->d_name;
            filenames += "\">";
            filenames += en->d_name;
            filenames += "</a><br>";
        }
    }
    else
        throw ErrorCodeClientException(client, 500, "Failed to open directory: " + client._rootPath + ", because: " + string(strerror(errno)));
    string htmlResponse = "<html><body><h1>Index of " + client._rootPath + "</h1><pre>" + filenames + "</pre></body></html>";
    string response = HttpResponse(client, 200, ".html", htmlResponse.size()) + htmlResponse;
    client._filenamePath.clear();
    closedir(dr); // TODO not protected
    unique_ptr handleClient = make_unique<HandleToClientTransfer>(client, response);
    RunServers::insertHandleTransfer(move(handleClient));
    Logger::log(INFO, client, "GET    ", client._requestPath);
}

void HttpRequest::processPost(Client &client)
{
    switch (client._headerParseState)
    {
        case REQUEST_READY:
            processHttpBody(client);
            break;
        case BODY_CHUNKED:
        {
            client._contentLength = 0;
            unique_ptr<HandleTransfer> handle = make_unique<HandleChunkTransfer>(client);
            if (handle->handleChunkTransfer() == true)
            {
                return;
            }
            RunServers::insertHandleTransfer(move(handle));
            break;
        }
    }
}

void HttpRequest::processDelete(Client &client)
{
    uint16_t code = 200;
    if (remove(('.' + client._requestPath).data()) == 0)
    {
        string body = "File deleted";
        string response = HttpRequest::HttpResponse(client, code, ".txt", body.size());
        response += body;
        unique_ptr handleClient = make_unique<HandleToClientTransfer>(client, response);
        RunServers::insertHandleTransfer(move(handleClient));
        Logger::log(INFO, client, "DELETE ", client._rootPath);
        // RunServers::clientHttpCleanup(client);
    }
    else
    {
        switch (errno)
        {
            case EACCES:
            case EPERM:
            case EROFS:
                code = 403;
                break;
            case ENOENT:
            case ENOTDIR:
                code = 404;
                break;
            case EISDIR:
                code = 405;
                break;
            default:
                code = 500;
        }
        throw ErrorCodeClientException(client, code, string("Remove failed: ") + strerror(errno));
    }
}

