#include <HttpRequest.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HandleTransfer.hpp>

bool validateMultipartPostSyntax(Client &client, string &buffer);

bool HttpRequest::processHttpBody(Client &client)
{
    HttpRequest::getContentLength(client);
    // if (true) // check if cgi request
    // {
    //     std::cout << "body is: "  <<  escape_special_chars(client._body) << std::endl; //testcout
    //     std::cout << "body size: " << client._body.size() << std::endl; //testcout
    //     if (client._body.find(string(client._bodyBoundary) + "--" + CRLF) == string::npos)
    //     {
    //         std::cout << "returning false"  << std::endl; //testcout
    //         return false;
    //     }
    //     if (validateMultipartPostSyntax(client, client._body) == false)
    //     {
    //         RunServers::logMessage(5, "POST request syntax error, clientFD: ", client._fd);
    //         throw ErrorCodeClientException(client, 400, "Malformed POST request syntax");
    //     }
    //     std::cout << "correct post request for cgi"  << std::endl; //testcout
    //     // send to cgi process
    //     return true;
    // }
    unique_ptr<HandleTransfer> handle;
    handle = make_unique<HandleTransfer>(client, client._body.size(), client._body);
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


// bool HttpRequest::processHttpChunkBody(Client &client, int targetFilePathFD)
// {
//     HttpRequest::getBodyInfo(client);
//     client._filenamePath = client._rootPath + "/" + string(client._filename); // here to append filename for post
//     int fd = open(client._filenamePath.data(), O_WRONLY | O_TRUNC | O_CREAT, 0700);
//     if (fd == -1)
//     {
//         if (errno == EACCES)
//             throw ErrorCodeClientException(client, 403, "access not permitted for post on file: " + client._filenamePath);
//         else
//             throw ErrorCodeClientException(client, 500, "couldn't open file because: " + string(strerror(errno)) + ", on file: " + client._filenamePath);
//     }
//     FileDescriptor::setFD(fd);
//     targetFilePathFD = fd;
//     return true;
// }

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
    

void HttpRequest::getBodyInfo(Client &client, const string buff)
{
    string contentDisposition = "Content-Disposition:";
    size_t cdPos = buff.find(contentDisposition);
    if (cdPos == string::npos)
        throw RunServers::ClientException("Content-Disposition header not found in multipart body");
    size_t formDataPos = buff.find("form-data", cdPos);
    
    // Find the amount of whitespace between "Content-Disposition:" and the next character
    size_t wsStart = cdPos + contentDisposition.size();
    size_t wsEnd = buff.find_first_not_of(" \t", wsStart);
    if (wsEnd == string::npos)
        throw ErrorCodeClientException(client, 400, "Malformed Content-Disposition header: only whitespace found");
    if (formDataPos != wsEnd)
        throw ErrorCodeClientException(client, 400, "Content-Disposition is not form-data");
    // Extract the Content-Disposition line
    size_t cdEnd = buff.find("\r\n", cdPos);
    // string_view cdLine = string_view(buff).substr(cdPos, cdEnd - cdPos);
    // string_view cdLine(buff.data(), cdEnd - cdPos);
    string cdLine = buff.substr(cdPos, cdEnd - cdPos);

    string filenameKey = "filename=\"";
    size_t fnPos = cdLine.find(filenameKey);
    if (fnPos != string::npos)
    {
        size_t fnStart = fnPos + filenameKey.size();
        size_t fnEnd = cdLine.find("\"", fnStart);
        client._filename = string(cdLine).substr(fnStart, fnEnd - fnStart);
        if (client._filename.empty())
            throw ErrorCodeClientException(client, 400, "Filename is empty in Content-Disposition header");
    }
    else
        throw ErrorCodeClientException(client, 400, "Filename not found in Content-Disposition header");
    const string contentType = "Content-Type: ";
    size_t position = buff.find(contentType); //TODO should we do something with contentType here?
    if (position == string::npos && !client._filename.empty())
        throw ErrorCodeClientException(client, 400, "Content-Type header not found in multipart body");
}

