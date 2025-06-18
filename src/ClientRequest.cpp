#include <RunServer.hpp>
#include <HttpRequest.hpp>
#include <Client.hpp>

Location &RunServers::setLocation(Client &client, unique_ptr<Server> &usedServer)
{
	size_t pos = string_view(client._header).find_first_not_of(" \t", client._method.length());
	if (pos == string::npos || client._header[pos] != '/')
		throw RunServers::ClientException("missing path in HEAD");
	size_t len = string_view(client._header).substr(pos).find_first_of(" \t\n\r");
	client._path = client._header.substr(pos, len);
	for (pair<string, Location> &locationPair : usedServer->getLocations())
	{
		if (strncmp(client._path.data(), locationPair.first.c_str(), locationPair.first.length()) == 0)
		{
            return locationPair.second;
		}
	}
	throw RunServers::ClientException("No matching location found for path: " + client._path);
}

void RunServers::processClientRequest(Client &client)
{
    try
    {
        char	buff[CLIENT_BUFFER_SIZE];
        ssize_t bytesReceived = recv(client._fd, buff, sizeof(buff), 0);
        if (bytesReceived < 0)
        {
            cleanupClient(client);
            if(errno == EINTR)
            {
                //	STOP SERVER CLEAN UP
            }
            else if (errno != EAGAIN)
                cerr << "recv: " << strerror(errno);
            return;
        }
        // ClientRequestState &state = _clientStates[clientFD];
        

        if (bytesReceived == 0) {
            if (client._headerParsed && client._method == "POST" && client._body.size() < client._contentLength) {
                sendErrorResponse(client._fd, "400 Bad Request (incomplete body)");
            }
            cleanupClient(client);
            return;
        }
        size_t receivedBytes = static_cast<size_t>(bytesReceived);
    
        client._fdBuffers.append(buff, receivedBytes); // can fail, need to call cleanupClient
        // std::cout << escape_special_chars(client._fdBuffers) << std::endl;

        // std::cout << "\t" << client.headerParsed << std::endl;
        if (client._headerParsed == false)
        {
            size_t headerEnd = client._fdBuffers.find("\r\n\r\n");
            if (headerEnd == string::npos)
            {
                sendErrorResponse(client._fd, "400 Bad Request");
                cleanupClient(client);
                return;
            }
    
            client._header = client._fdBuffers.substr(0, headerEnd + 4); // can fail, need to call cleanupClient
            client._body = client._fdBuffers.substr(headerEnd + 4); // can fail, need to call cleanupClient
            client._headerParsed = true;
            
            // Parse Content-Length if POST
            client._method = extractMethod(client._header); // can fail WITHIN FUNCTION, need to call cleanupClient, and be able to call it there
            // std::cout << client.contentLength << std::endl;
            unique_ptr<Server> usedServer;
            setServer(client, usedServer);
            Location &loc = setLocation(client, usedServer);

            if (client._method.empty() == true)
            {
                sendErrorResponse(client._fd, "400 Bad Request");
                cleanupClient(client);
                return;
            }
            // std::cout << "." << client.method << "." << std::endl;
            // exit(0);

            // if (client.method == "POST")
            if (strncmp(client._method.data(), "POST", 4) == 0)
            {
                // std::cout << client.method << std::endl;
                string lengthStr = extractHeader(client._header, "Content-Length:");
                client._contentLength = headerNameContentLength(lengthStr, loc.getClientBodySize());
            }


        } else {
            parseHeaders(client);
            // size_t boundaryPos = client.body.find_first_of()
            // Only append new data to body
            client._body.append(buff, bytesReceived);
        }
        if (client._method == "POST" && client._body.size() < client._contentLength)
            return; // Wait for more data


// std::cout << escape_special_chars(client._fdBuffers) << std::endl;
// std::cout << escape_special_chars(client.header) << std::endl;

        unique_ptr<Server> usedServer;
        setServer(client, usedServer);
        Location &loc = setLocation(client, usedServer);
        HttpRequest request(usedServer, loc, client._fd, client);

        request.handleRequest(client._contentLength);
        client._fdBuffers.clear();
        // client.clear();
        client._headerParsed = false;
        client._header = "";
        client._body = "";
        client._path = "";
        client._method = "";
        client._contentLength = 0;
    }
    catch(const exception& e)   // if catch we don't handle well
    {
        cerr << e.what() << endl;
        string msgToClient = "400 Bad Request, <html><body><h1>400 Bad Request</h1></body></html>";
        // exit(0);
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

    // client._fdBuffers.clear();
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

void RunServers::setServer(Client &client, unique_ptr<Server> &usedServer)
{
    uint find = client._header.find("Host:") + 5;
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

void RunServers::cleanupClient(Client &client)
{
    _clients.erase(client._fd);
    _connectedClients.erase(remove(_connectedClients.begin(), _connectedClients.end(), client._fd), _connectedClients.end());
    client._fdBuffers.clear();
    cleanupFD(client._fd);
}

void RunServers::parseHeaders(Client &client)
{
    size_t start = 0;
    while (start < client._header.size())
    {
        size_t end = client._header.find("\r\n", start);
        if (end == string::npos)
            throw RunServers::ClientException("Malformed HTTP request: header line not properly terminated");

        string_view line(&client._header[start], end - start);
        if (line.empty())
            break; // End of headers

        size_t colon = line.find(':');
        if (colon != string_view::npos) {
            // Extract key and value as string_views
            string_view key = line.substr(0, colon);
            string_view value = line.substr(colon + 1);

            // Trim whitespace from key
            key.remove_prefix(key.find_first_not_of(" \t"));
            key.remove_suffix(key.size() - key.find_last_not_of(" \t") - 1);
            // Trim whitespace from value
            value.remove_prefix(value.find_first_not_of(" \t"));
            value.remove_suffix(value.size() - value.find_last_not_of(" \t") - 1);
            
            // _headerFields[string(key)] = value;
            client._headerFields[string(key)] = value;
            // cout << "\tkey\t" << key << "\t" << value << endl;
        }
        start = end + 2;
    }
}