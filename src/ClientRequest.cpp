#include <RunServer.hpp>
#include <HttpRequest.hpp>
#include <Client.hpp>
#include <ErrorCodeClientException.hpp>
#include "Logger.hpp"


void    RunServers::setLocation(Client &client)
{
	for (pair<string, Location> &locationPair : client._usedServer->getLocations())
	{
		if (strncmp(client._requestPath.data(), locationPair.first.data(), locationPair.first.size()) == 0 &&
        (client._requestPath[client._requestPath.size()] == '\0' || client._requestPath[locationPair.first.size() - 1] == '/'))
		{
            client._location = locationPair.second;
            return;
        }
    }
    throw ErrorCodeClientException(client, 400, "Couldn't find location block: malformed request");
}

void RunServers::processClientRequest(Client &client)
{
    try
    {
        char   buff[_clientBufferSize];
        size_t bytesReceived = receiveClientData(client, buff);
        static bool (*const handlers[4])(Client&, const char*, size_t) = {
            &HttpRequest::parseHttpHeader,                     // HEADER_AWAITING (0)
            &HttpRequest::appendToBody,                        // BODY_CHUNKED (1)
            &HttpRequest::parseHttpBody,                       // BODY_AWAITING (2)
            [](Client&, const char*, size_t) { return true; }  // REQUEST_READY (3)
        };
        if (handlers[client._headerParseState](client, buff, bytesReceived) == false)
            return ;
        // client._finishedProcessClientRequest = true;
        HttpRequest::handleRequest(client);
    }
    catch(const exception& e)   // if catch we don't handle well
    {
        Logger::log(ERROR, client, "Error processing client request: ", e.what());
        string msgToClient = "400 Bad Request, <html><body><h1>400 Bad Request</h1></body></html>";
        sendErrorResponse(client._fd, msgToClient);
    }
    // catch (const LengthRequiredException &e)
    // {
    //     cerr << e.what() << endl;
    //     sendErrorResponse(clientFD, "411 Length Required");
    // }
    // catch (const ClientException &e)
    // {
    //     cerr << e.what() << endl;
    //     sendErrorResponse(clientFD, "400 Bad Request");
    // }
    // cleanupClient(clientFD);
}

size_t RunServers::receiveClientData(Client &client, char *buff)
{
    // buff[_clientBufferSize] = '\0'; // kan alleen aan voor testen anders kan het voor post problemen geven
    client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    errno = 0;
    ssize_t bytesReceived = recv(client._fd, buff, _clientBufferSize, 0);
    // std::cout << "received: " << escape_special_chars(string(buff, bytesReceived)) << std::endl; //testcout
    if (bytesReceived > 0)
        return static_cast<size_t>(bytesReceived);
    if (bytesReceived < 0)
    {
        cerr << "recv: " << strerror(errno);
        RunServers::cleanupClient(client);
        throw runtime_error("something"); // TODO need new exception. send no response, just cleanup and maybe log
    }
    if (bytesReceived == 0)
    {
        throw ErrorCodeClientException(client, 0, "kicking out client after read of 0"); // todo find different solution maybe
    }
    return (0); // You never get here
}

string extractHeader(const string &header, const string &key)
{
    size_t start = header.find(key);

    if (start == string::npos)
        return "";
    start += key.length() + 1;
    size_t endPos = header.find("\r\n", start);

    if (start == string::npos)
        return "";
    return header.substr(start, endPos - start);
}

void sendErrorResponse(int clientFD, const string &message)
{
    string response = "HTTP/1.1 " + message + "\r\nContent-Length: 0\r\n\r\n";
    send(clientFD, response.c_str(), response.size(), MSG_NOSIGNAL);
}

void RunServers::clientHttpCleanup(Client &client)
{
    client._headerParseState = HEADER_AWAITING;
    client._header.clear();
    client._body.clear();
    client._requestPath.clear();
    client._queryString.clear();
    client._method.clear();
    client._useMethod = 0;
    client._contentLength = 0;
    client._headerFields.clear();
    client._rootPath.clear();
	client._filenamePath.clear();
    client._name.clear();
    client._version.clear();
    client._bodyEnd = 0;
    client._filename.clear();
    client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    client._isAutoIndex = false;
    client._isCgi = false;
}



void RunServers::cleanupClient(Client &client)
{
    int clientFD = client._fd;
    Logger::log(INFO, client, "Disconnected");
    for (auto it = _handle.begin(); it != _handle.end();)
    {
        if ((*it)->_client._fd == clientFD)
            it = _handle.erase(it);
        else
            ++it;
    }
    _clients.erase(clientFD);
    FileDescriptor::cleanupFD(clientFD);
}
