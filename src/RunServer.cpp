#include <RunServer.hpp>
#include <iostream>
#include <FileDescriptor.hpp>
#include <HttpRequest.hpp>

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // close
#include <stdlib.h>	// callod
#ifdef __linux__
#include <sys/epoll.h>
#endif
#include <sys/socket.h>

#include <dirent.h>

#include <signal.h>
extern volatile sig_atomic_t g_signal_status;

// Static member variables
FileDescriptor RunServers::_fds;
int RunServers::_epfd = -1;
array<struct epoll_event, FD_LIMIT> RunServers::_events;
unordered_map<int, string> RunServers::_fdBuffers;
unordered_map<int, ClientRequestState> RunServers::_clientStates;
ServerList RunServers::_servers;

RunServers::~RunServers()
{
    close(_epfd);
}

int RunServers::make_socket_non_blocking(int sfd)
{
    int currentFlags = fcntl(sfd, F_GETFL, 0);
    if (currentFlags == -1)
    {
        cerr << "fcntl: " << strerror(errno);
        return -1;
    }

    currentFlags |= O_NONBLOCK;
    int fcntlResult = fcntl(sfd, F_SETFL, currentFlags);
    if (fcntlResult == -1)
    {
        cerr << "fcntl: " << strerror(errno);
        return -1;
    }
    return 0;
}

int RunServers::epollInit(/* ServerList &servers */)
{
    _epfd = epoll_create(FD_LIMIT); // parameter must be bigger than 0, rtfm
    if (_epfd == -1)
{
        cerr << "Server epoll_create: " << strerror(errno);
        return -1;
    }
	vector<int> seenInts;
    for (const unique_ptr<Server> &server : _servers)
    {
        struct epoll_event current_event;
		current_event.events = EPOLLIN | EPOLLET;
		for (int listener : server->getListeners())
		{
			current_event.data.fd = listener;
			if (find(seenInts.begin(), seenInts.end(), listener) != seenInts.end())
				continue ;
			if (epoll_ctl(_epfd, EPOLL_CTL_ADD, listener, &current_event) == -1)
			{
				cerr << "Server epoll_ctl: " << strerror(errno) << endl;
				close(_epfd);
				return -1;
			}
			seenInts.push_back(listener);
		}
    }
    return 0;
}

void RunServers::createServers(vector<ConfigServer> &configs)
{
	for (ConfigServer &config : configs)
	{
		_servers.push_back(make_unique<Server>(Server(config)));
	}
	Server::createListeners(_servers);
	// for (unique_ptr<Server> &server : _servers)
	// {
	// 	server->getConfig().
	// }
}

int RunServers::runServers()
{
    try
    {
        while (g_signal_status == 0)
        {
            int eventCount;
    
            fprintf(stdout, "Blocking and waiting for epoll event...\n");
            eventCount = epoll_wait(_epfd, _events.data(), FD_LIMIT, -1);
            if (eventCount == -1) // for use only goes wrong with EINTR(signals)
            {
                break ;
                throw runtime_error(string("Server epoll_wait: ") + strerror(errno));
            }
            // fprintf(stdout, "Received epoll event\n");
            handleEvents(static_cast<size_t>(eventCount));
        }
        cout << "\rGracefully stopping... (press Ctrl+C again to force)" << endl;
    }
    catch(const exception& e) // catch-all, will determine whether CleanupClient needs to be called or not
    {
        cerr << e.what() << endl;
    }

    return 0;
}

void RunServers::handleEvents(size_t eventCount)
{
    // int errHndl = 0;
    for (size_t i = 0; i < eventCount; ++i)
    {
        struct epoll_event &currentEvent = _events[i];
        // if ((currentEvent.events & EPOLLERR) ||
        //     (currentEvent.events & EPOLLHUP) ||
        //     (currentEvent.events & EPOLLIN) == 0)
        // {
        if ((currentEvent.events & (EPOLLERR | EPOLLHUP))||
           !(currentEvent.events & EPOLLIN))
        {
            std::cerr << "epoll error on fd " << currentEvent.data.fd
            << " (events: " << currentEvent.events << ")" << std::endl;
            cleanupFD(currentEvent.data.fd);
            continue;
        }

		// bool found = false;
		int clientFD = currentEvent.data.fd;
        for (const unique_ptr<Server> &server : _servers)
        {
			vector<int> &listeners = server->getListeners();
            if (find(listeners.begin(), listeners.end(), clientFD) != listeners.end())
            {
				// found = true;
                acceptConnection(server);
            }
            else if (server == _servers.back())
            {
                processClientRequest(server, clientFD);
            }
        }
		// if (found == false)
		// 	processClientRequest(server, clientFD);
    }
}

void RunServers::acceptConnection(const unique_ptr<Server> &server)
{
    while (true)
    {
        socklen_t in_len = sizeof(struct sockaddr);
        struct sockaddr in_addr;
        int infd = accept(server->getListeners()[0], &in_addr, &in_len); // TODO does it matter if server accepts on different listener fd than what it was caught on?
        if(infd == -1)
        {
            if(errno != EAGAIN)
                perror("accept");
            break;
        }
        

        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
        if(getnameinfo(&in_addr, in_len, hbuf, sizeof(hbuf), sbuf, 
            sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
        {
            printf("%s: Accepted connection on descriptor %d"
                "(host=%s, port=%s)\n", server->getConfig().getServerName().c_str(), infd, hbuf, sbuf);
        }
        if(make_socket_non_blocking(infd) == -1)
        {
            close(infd);
            break;
        }
        
        struct epoll_event  current_event;
        current_event.data.fd = infd;
        current_event.events = EPOLLIN /* | EPOLLET */; // EPOLLET niet gebruiken, stopt meerdere pakketen verzende
        if(epoll_ctl(_epfd, EPOLL_CTL_ADD, infd, &current_event) == -1)
        {
            cout << "epoll_ctl: " << strerror(errno) << endl;
            close(infd);
            break;
        }
        _fds.setFD(infd);

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

static string NumIpToString(uint32_t addr)
{
	uint8_t num;
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
		for (pair<const string, string> &porthost : server->getConfig().getPortHost())
		{
			if (porthost.second.find("0.0.0.0") != string::npos || 
				porthost.second.find(ip) != string::npos)
			{
				if (to_string(port) == porthost.first)
				{
					if (hostname == server->getConfig().getServerName())
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

void RunServers::processClientRequest(const unique_ptr<Server> &server, int clientFD)
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

		std::cout << escape_special_chars(state.header) << std::endl;

		unique_ptr<Server> usedServer;
		parseHost(state.header, clientFD, usedServer);
		std::cout << "server name:" << usedServer->getConfig().getServerName() << std::endl;
        HttpRequest request(usedServer, clientFD, state.method, state.header, state.body);
        request.handleRequest(state.contentLength);
        printf("%s: Closed connection on descriptor %d\n", server->getConfig().getServerName().c_str(), clientFD);
    }
    catch (const LengthRequiredException &e)
    {
        cerr << e.what() << endl;
        sendErrorResponse(clientFD, "411 Length Required");
    }
    catch (const ClientException &e)
    {
        cerr << e.what() << endl;
        sendErrorResponse(clientFD, "400 Bad Request");
    }
    catch(const exception& e)
    {
        cerr << e.what() << endl;
        string msgToClient = "400 Bad Request, <html><body><h1>400 Bad Request</h1></body></html>";
        sendErrorResponse(clientFD, msgToClient);
    }
    cleanupClient(clientFD);
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

    if (start == string::npos) return "";
    start += key.length() + 1;
    size_t endPos = header.find("\r\n", start);

    if (start == string::npos) return "";
    return header.substr(start, endPos - start);
}

void sendErrorResponse(int clientFD, const string &message)
{
    string response = "HTTP/1.1 " + message + "\r\nContent-Length: 0\r\n\r\n";
    send(clientFD, response.c_str(), response.size(), 0);
}

void RunServers::cleanupFD(int fd)
{
    if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        cerr << "epoll_ctl: EPOLL_CTL_DEL: " << strerror(errno) << endl;
    }
    _fds.closeFD(fd);
}

void RunServers::cleanupClient(int clientFD)
{
    _clientStates.erase(clientFD);
    _fdBuffers[clientFD].clear();
    cleanupFD(clientFD);
}
