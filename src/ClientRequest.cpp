#include <RunServer.hpp>
#include <HttpRequest.hpp>


void RunServers::processClientRequest(int clientFD)
{
    try
    {
        char	buff[CLIENT_BUFFER_SIZE];
        ssize_t bytesReceived = recv(clientFD, buff, sizeof(buff), 0);
        
        if (bytesReceived < 0)
        {
            cleanupClient(clientFD);
            if(errno == EINTR)
            {
                //	STOP SERVER CLEAN UP
            }
            else if (errno != EAGAIN)
                cerr << "recv: " << strerror(errno);
            return;
        }
        ClientRequestState &state = _clientStates[clientFD];
    
        if (bytesReceived == 0) {
            if (state.headerParsed && state.method == "POST" && state.body.size() < state.contentLength) {
                sendErrorResponse(clientFD, "400 Bad Request (incomplete body)");
            }
            cleanupClient(clientFD);
            return;
        }
        size_t receivedBytes = static_cast<size_t>(bytesReceived);
    
        _fdBuffers[clientFD].append(buff, receivedBytes); // can fail, need to call cleanupClient
    
        if (state.headerParsed == false)
        {
            size_t headerEnd = _fdBuffers[clientFD].find("\r\n\r\n");
            if (headerEnd == string::npos)
            {
                sendErrorResponse(clientFD, "400 Bad Request");
                cleanupClient(clientFD);
                return;
            }
    
            state.header = _fdBuffers[clientFD].substr(0, headerEnd + 4); // can fail, need to call cleanupClient
            state.body = _fdBuffers[clientFD].substr(headerEnd + 4); // can fail, need to call cleanupClient
            state.headerParsed = true;
    
            // Parse Content-Length if POST
            state.method = extractMethod(state.header); // can fail WITHIN FUNCTION, need to call cleanupClient, and be able to call it there
            if (state.method.empty() == true)
            {
                sendErrorResponse(clientFD, "400 Bad Request");
                cleanupClient(clientFD);
                return;
            }
            if (state.method == "POST")
            {
                string lengthStr = extractHeader(state.header, "Content-Length:");
                state.contentLength = headerNameContentLength(lengthStr, 1024*1024*100/* client_max_body_size */);
            }

        } else {
            // Only append new data to body
            state.body.append(buff, bytesReceived);
        }
        
        if (state.method == "POST" && state.body.size() < state.contentLength)
            return; // Wait for more data
        unique_ptr<Server> usedServer;
        parseHost(state.header, clientFD, usedServer);
        HttpRequest request(usedServer, clientFD, state.method, state.header, state.body);
        request.handleRequest(state.contentLength);
    }
    catch(const exception& e)
    {
        cerr << e.what() << endl;
        string msgToClient = "400 Bad Request, <html><body><h1>400 Bad Request</h1></body></html>";
        sendErrorResponse(clientFD, msgToClient);
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

void RunServers::parseHost(string &header, int clientFD, unique_ptr<Server> &usedServer)
{
    uint find = header.find("Host:") + 5;
    string_view hostname = string_view(header).substr(find);
    hostname.remove_prefix(hostname.find_first_not_of(" \t"));
    size_t len = hostname.find_first_of(" \t\n\r");
    hostname = hostname.substr(0, len);

    sockaddr_in res;
    socklen_t resLen = sizeof(res);
    int err = getsockname(clientFD, (struct sockaddr*)&res, &resLen);
    if (err != 0)
        throw ClientException("getsockname failed: " + string(strerror(errno)));
    string ip = NumIpToString(static_cast<uint32_t>(res.sin_addr.s_addr));
    uint16_t port = htons(static_cast<uint16_t>(res.sin_port));
    usedServer = nullptr;
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
                        usedServer = make_unique<Server>(*server);
                        return;
                    }
                    if (usedServer == nullptr)
                        usedServer = make_unique<Server>(*server);
                    break ;
                }
            }
        }
    }
}

size_t RunServers::headerNameContentLength(const std::string &length, size_t client_max_body_size)
{
    if (length.empty())
    {
        throw LengthRequiredException("Content-Length header is empty.");
    }

    for (size_t i = 0; i < length.size(); ++i)
    {
        if (!isdigit(static_cast<unsigned char>(length[i])))
            throw ClientException("Content-Length contains non-digit characters.");
    }
    long long value;
    try
    {
        value = std::stoll(length);

        if (value < 0)
            throw ClientException("Content-Length cannot be negative.");

        if (static_cast<size_t>(value) > client_max_body_size)
            throw ClientException("Content-Length exceeds maximum allowed."); // (413, "Payload Too Large");

        if (value == 0)
            throw ClientException("Content-Length cannot be zero.");
    }
    catch (const std::invalid_argument &)
    {
        throw ClientException("Content-Length is invalid (not a number).");
    }
    catch (const std::out_of_range &)
    {
        throw ClientException("Content-Length value is out of range.");
    }
    return (static_cast<size_t>(value));
}

string extractMethod(const string &header)
{
    size_t methodEnd = header.find(" ");

    if (methodEnd == string::npos) return "";
    return header.substr(0, methodEnd);
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

void RunServers::insertClientFD(int fd)
{
    _connectedClients.push_back(fd);
}

void RunServers::cleanupFD(int fd)
{
    if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        cerr << "epoll_ctl: " << strerror(errno) << endl;
    }
    _fds.closeFD(fd);
}

void RunServers::cleanupClient(int clientFD)
{
    _clientStates.erase(clientFD);
    _fdBuffers[clientFD].clear();
    cleanupFD(clientFD);
}
