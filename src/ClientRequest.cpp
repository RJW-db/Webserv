#include <RunServer.hpp>
#include <HttpRequest.hpp>
#include <Client.hpp>
#include <ErrorCodeClientException.hpp>

void    RunServers::setLocation(Client &client)
{
    size_t pos = string_view(client._header).find_first_not_of(" \t", client._method.size());
    if (pos == string::npos || client._header[pos] != '/')
        throw RunServers::ClientException("missing path in HEAD");
    size_t len = string_view(client._header).substr(pos).find_first_of(" \t\n\r");
    client._path = client._header.substr(pos, len);
    for (pair<string, Location> &locationPair : client._usedServer->getLocations())
    {
        if (strncmp(client._path.data(), locationPair.first.data(), locationPair.first.size()) == 0 && 
        (client._path[client._path.size() - 1] == '\0' || client._path[locationPair.first.size() - 1] == '/'))
        {
            client._location = locationPair.second;
            return;
        }
    }
    throw RunServers::ClientException("No matching location found for path: " + client._path);
}

void RunServers::processClientRequest(Client &client)
{
    try
    {
        char   buff[CLIENT_BUFFER_SIZE];
        size_t bytesReceived = receiveClientData(client, buff);

        static bool (*const handlers[])(Client&, const char*, size_t) = {
            &HttpRequest::parseHttpHeader,                     // HEADER_NOT_PARSED (0)
            &HttpRequest::parseHttpBody,                       // HEADER_PARSED_POST (1)
            [](Client&, const char*, size_t) { return true; } // HEADER_PARSED_NON_POST (2)
        };
        if (handlers[client._headerParseState](client, buff, bytesReceived) == false)
            return;

        HttpRequest::handleRequest(client);
        clientHttpCleanup(client);
    }
    catch(const exception& e)   // if catch we don't handle well
    {
        cerr << e.what() << endl;
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
    ssize_t bytesReceived = recv(client._fd, buff, CLIENT_BUFFER_SIZE, 0);
    if (bytesReceived < 0)
    {
        cerr << "recv: " << strerror(errno);
        RunServers::cleanupClient(client);
                            throw runtime_error("something"); // TODO need new exception. send no response, just cleanup and maybe log
    }

    return static_cast<size_t>(bytesReceived);
}

static string NumIpToString(uint32_t addr)
{
    uint32_t num;
    string result;
    int bitshift = 24;
    uint32_t rev_addr = htonl(addr);

    for (uint8_t i = 0; i < 4; i++)
    {
        num = rev_addr >> bitshift;
        result = result + to_string(num);
        rev_addr &= ~(255 >> bitshift);
        if (i != 3)
            result += ".";
        bitshift -= 8;
    }
    return result;
}

void RunServers::setServer(Client &client)
{
    uint find = client._header.find("Host:") + 5;       // TODO uint sam?
    string_view hostname = string_view(client._header).substr(find);
    hostname.remove_prefix(hostname.find_first_not_of(" \t"));
    size_t len = hostname.find_first_of(" \t\n\r");
    hostname = hostname.substr(0, len);

    sockaddr_in res;
    socklen_t resLen = sizeof(res);
    int err = getsockname(client._fd, (struct sockaddr*)&res, &resLen);
    if (err != 0)
        throw ClientException("getsockname failed: " + string(strerror(errno)));
    string ip = NumIpToString(static_cast<uint32_t>(res.sin_addr.s_addr));
    uint16_t port = htons(static_cast<uint16_t>(res.sin_port));
    client._usedServer = nullptr;
    for (unique_ptr<Server> &server : _servers)
    {
        for (pair<const string, string> &porthost : server->getPortHost())
        {
            if (porthost.second.find("0.0.0.0") != string::npos || 
                porthost.second.find(ip) != string::npos)
            {
                if (to_string(port) == porthost.first)
                {
                    if (hostname == server->getServerName())
                    {
                        client._usedServer = make_unique<Server>(*server);
                        return;
                    }
                    if (client._usedServer == nullptr)
                        client._usedServer = make_unique<Server>(*server);
                    break ;
                }
            }
        }
    }
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
    send(clientFD, response.c_str(), response.size(), 0);
}

// void RunServers::insertClientFD(int fd)
// {
//     _connectedClients.push_back(fd);
// }

void RunServers::clientHttpCleanup(Client &client)
{
    client._headerParseState = HEADER_NOT_PARSED;
    client._header.clear();
    client._body.clear();
    client._path.clear();
    client._method.clear();
    client._contentLength = 0;
    client._headerFields.clear();
    client.setDisconnectTime(disconnectDelaySeconds);
}

void RunServers::cleanupFD(int fd)
{
    if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        cerr << "epoll_ctl: " << strerror(errno) << endl;
    }
    _fds.closeFD(fd);
}

void RunServers::cleanupClient(Client &client)
{
    // _connectedClients.erase(remove(_connectedClients.begin(), _connectedClients.end(), client._fd), _connectedClients.end());
    cleanupFD(client._fd);
    _clients.erase(client._fd);
}

// unique_ptr<Client> &RunServers::getClient(int clientFd) 