#include <RunServer.hpp>
#include <HttpRequest.hpp>
#include <Client.hpp>

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
        (client._path[client._path.size()] == '\0' || client._path[locationPair.first.size() - 1] == '/'))
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
                return;
            }
            client._headerParsed = true;
            client._header = client._fdBuffers.substr(0, headerEnd + 4); // can fail, need to call cleanupClient
            client._body = client._fdBuffers.substr(headerEnd + 4); // can fail, need to call cleanupClient
            client._fdBuffers.clear();
    
    
            HttpRequest::validateHEAD(client);  // TODO cleanupClient
            HttpRequest::parseHeaders(client);  // TODO cleanupClient
    
    
            setServer(client);
            setLocation(client);
        }
		else
			client._body.append(buff, receivedBytes);
        if (client._method == "POST")
        {
            size_t bodyEnd = client._body.find("\r\n\r\n");
			if (bodyEnd == string::npos)
				return;
			auto contentLength = client._headerFields.find("Content-Length");
            if (contentLength == client._headerFields.end())
                throw ClientException("Broken POST request");
			HttpRequest::getContentLength(client, contentLength->second);
			
			HttpRequest::getBodyInfo(client);
			auto it = client._headerFields.find("Content-Type");
			if (it == client._headerFields.end())
				throw RunServers::ClientException("Missing Content-Type");
   			ContentType ct = HttpRequest::getContentType(client, it->second);
			size_t writableContentLength = client._contentLength - bodyEnd - 4 - client._bodyBoundary.size() - 4 - 2 - 2; // bodyend - 4(\r\n\r\n) bodyboundary (-4 for - signs) - 2 (\r\n) - 2 (\r\n)

			
            string content = client._body.substr(bodyEnd + 4);
            int fd = open("./upload/image.png", O_WRONLY | O_TRUNC | O_CREAT, 0700);
            // int fd = open("./upload/test.txt", O_WRONLY | O_TRUNC | O_CREAT, 0700);
            if (fd == -1)
                exit(0);
            FileDescriptor::setFD(fd);
			size_t writeSize = writableContentLength;
			if (content.size() < writableContentLength)
				writeSize = content.size();
            ssize_t bytesWritten = write(fd, content.data(), writeSize);
			unique_ptr<HandleTransfer> handle;
            if (bytesWritten == writableContentLength)
            {
                if (content.find("--" + string(client._bodyBoundary) + "--\r\n") == writableContentLength + 2)
                {
                    FileDescriptor::closeFD(fd);
					// Fix: Complete HTTP response with proper headers
					string ok = HttpRequest::HttpResponse(200, "", 0);
					send(client._fd, ok.data(), ok.size(), 0);
					return ;
				}
				handle = make_unique<HandleTransfer>(client, fd, static_cast<size_t>(bytesWritten), writableContentLength, content.substr(bytesWritten));
			}
			else
				handle = make_unique<HandleTransfer>(client, fd, static_cast<size_t>(bytesWritten), writableContentLength, "");
			insertHandleTransfer(move(handle));
		}


        // if (client._method == "POST" && client._body.size() < client._contentLength)
        //     return; // Wait for more data


// std::cout << escape_special_chars(client._fdBuffers) << std::endl;
// std::cout << escape_special_chars(client.header) << std::endl;

        // HttpRequest request(client._usedServer, client._location, client._fd, client);

        HttpRequest::handleRequest(client);
        client._headerParsed = false;
        client._header.clear();
        client._body.clear();
        client._path.clear();
        client._method.clear();
        client._contentLength = 0;
        client._headerFields.clear();
		client._fdBuffers.clear();
		client.setDisconnectTime(disconnectDelaySeconds);
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

void RunServers::setServer(Client &client)
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
    client._fdBuffers.clear();
    cleanupFD(client._fd);
    _clients.erase(client._fd);
}

// unique_ptr<Client> &RunServers::getClient(int clientFd) 