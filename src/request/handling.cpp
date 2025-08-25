#include "HttpRequest.hpp"

void HttpRequest::handleRequest(Client &client)
{
    if (client._location.getReturnRedirect().first > 0)
    {
        // cout << "entered return redirect: " << client._location.getReturnRedirect().first << endl; //testcout
        redirectRequest(client);
        RunServers::clientHttpCleanup(client);
        return ;
    }

    if (client._isCgi && client._useMethod != METHOD_POST)
    {
        handleCgi(client, client._body); // for GET second parameter will be unused
        RunServers::clientHttpCleanup(client);
        return;
    }
    switch (client._useMethod)
    {
    case METHOD_HEAD:
    {
        string response = HttpRequest::HttpResponse(client, 200, "txt", 0);
        send(client._fd, response.data(), response.size(), MSG_NOSIGNAL);
        break;
    }
    case METHOD_GET:
    {
        if (client._rootPath.find("/favicon.ico") != string::npos)
            client._rootPath = client._rootPath.substr(0, client._rootPath.find("/favicon.ico")) + "/favicon.svg";

        GET(client);
        break;
    }
    case METHOD_POST:
    {
        switch (client._headerParseState)
        {
            case REQUEST_READY:
            {
                processHttpBody(client);
                break;
            }
            case BODY_CHUNKED:
            {
                // handleChunks(client);
                // client._contentLength = client._location.getClientMaxBodySize();
                client._contentLength = 0;
                unique_ptr<HandleTransfer> handle;
                handle = make_unique<HandleChunkTransfer>(client);
                // handle->_client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
                if (handle->handleChunkTransfer() == true)
                {
                    if (client._keepAlive == false)
                        RunServers::cleanupClient(client);
                    else
                    {
                        RunServers::clientHttpCleanup(client);
                    }
                    return;
                }
                RunServers::insertHandleTransfer(move(handle));
            }
        }
        
        break;
    }
    case METHOD_DELETE:
    {
        int code = 200;
        if (remove(('.' + client._requestPath).data()) == 0)
        {
            string body = "File deleted";
            string response = HttpRequest::HttpResponse(client, code, ".txt", body.size());
            response += body;
            send(client._fd, response.data(), response.size(), MSG_NOSIGNAL);
            cout << escapeSpecialChars(response.c_str(), TERMINAL_DEBUG) << endl; //testcout
            Logger::log(INFO, client, "DELETE ", client._rootPath);
            RunServers::clientHttpCleanup(client);
        }
        else
        {
            switch (errno)
            {
            case EACCES:
            case EPERM:
            case EROFS:
                code = 403; // Forbidden";
                break;
            case ENOENT:
            case ENOTDIR:
                code = 404; // Not Found";
                break;
            case EISDIR:
                // if (limit_except == Doesn't allow DELETE)
                code = 405; // Method Not Allowed";
                // else
                // code = 403; // Forbidden";
                break;
            default:
                code = 500; // Internal Server Error;

                /**
                 * Status codes already been handled
                 * 414 - ENAMETOOLONG - URI Too Long
                 */
            }
            throw ErrorCodeClientException(client, code, string("Remove failed: ") + strerror(errno));
        }
        break;
    }
    default:
        throw ErrorCodeClientException(client, 405, "Method Not Allowed: " + client._method);
    }
}

void HttpRequest::redirectRequest(Client &client)
{
    pair<uint16_t, string> redirectPair = client._location.getReturnRedirect();
    string response = HttpResponse(client, redirectPair.first, "", 0);
    size_t index = response.find_first_of(CRLF);
    string locationHeader = "Location: " + redirectPair.second + CRLF;
    response.insert(index + CRLF_LEN, locationHeader);
    int sent = send(client._fd, response.data(), response.size(), 0);
    if (sent == -1)
        throw ErrorCodeClientException(client, 500, "Failed to send redirect response: " + string(strerror(errno)));
    
}

void HttpRequest::GET(Client &client)
{
    // locateRequestedFile(client);
    if (client._isAutoIndex == true)   // TODO check, is this check every possible at all
    {
        SendAutoIndex(client);
        RunServers::clientHttpCleanup(client);
        return ;
    }

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
    auto handleClient = make_unique<HandleToClientTransfer>(client, response);
    RunServers::insertHandleTransfer(move(handleClient));
    Logger::log(INFO, client, "GET    ", client._requestPath);
    if (client._keepAlive == false)
        RunServers::cleanupClient(client);
}

