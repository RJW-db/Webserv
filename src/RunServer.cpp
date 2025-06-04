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
int RunServers::_epfd = -1;
array<struct epoll_event, FD_LIMIT> RunServers::_events;
unordered_map<int, string> RunServers::_fdBuffers;
unordered_map<int, ClientRequestState> RunServers::_clientStates;

RunServers::RunServers(tmp_t *serverConf)
{
    ServerListenFD listenerFD(serverConf->port, "0.0.0.0");
    _serverName = serverConf->hostname;
    _listener = listenerFD.getFD();
}


RunServers::~RunServers()
{
    close(_listener);
    close(_epfd);
}

bool RunServers::directoryCheck(string &path)
{
    DIR *d = opendir(path.c_str());	// path = rde-brui
    if (d == NULL) {
        perror("opendir");
        return false;
    }

    // struct dirent *directoryEntry;
    // while ((directoryEntry = readdir(d)) != NULL) {
    //     printf("%s\n", directoryEntry->d_name);
    //     if (string(directoryEntry->d_name) == path)
    //     {
    //         closedir(d);
    //         return (true);
    //     }
    // }
    
    closedir(d);
    return (true);
    // return (false);
}

int RunServers::make_socket_non_blocking(int sfd)
{
    int currentFlags = fcntl(sfd, F_GETFL, 0);
    if (currentFlags == -1)
    {
        std::cerr << "fcntl: " << strerror(errno);
        return -1;
    }

    currentFlags |= O_NONBLOCK;
    int fcntlResult = fcntl(sfd, F_SETFL, currentFlags);
    if (fcntlResult == -1)
    {
        std::cerr << "fcntl: " << strerror(errno);
        return -1;
    }
    return 0;
}

int RunServers::epollInit(ServerList &servers)
{
    _epfd = epoll_create(FD_LIMIT); // parameter must be bigger than 0, rtfm
    if (_epfd == -1)
    {
        std::cerr << "Server epoll_create: " << strerror(errno);
        return -1;
    }

    for (const std::unique_ptr<RunServers> &server : servers)
    {
        struct epoll_event current_event;
        current_event.data.fd = server->_listener;
        current_event.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(_epfd, EPOLL_CTL_ADD, server->_listener, &current_event) == -1)
        {
            std::cerr << "Server epoll_ctl: " << strerror(errno) << std::endl;
            close(_epfd);
            return -1;
        }
    }
    return 0;
}

int RunServers::runServers(ServerList &servers, FileDescriptor &fds)
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
            handleEvents(servers, fds, static_cast<size_t>(eventCount));
        }
        cout << "\rGracefully stopping... (press Ctrl+C again to force)" << endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << endl;
    }

    return 0;
}

void RunServers::handleEvents(ServerList &servers, FileDescriptor &fds, size_t eventCount)
{
    // int errHndl = 0;
    for (size_t i = 0; i < eventCount; ++i)
    {
        struct epoll_event &currentEvent = _events[i];
        if ((currentEvent.events & EPOLLERR) ||
            (currentEvent.events & EPOLLHUP) ||
            (currentEvent.events & EPOLLIN) == 0)
        {
            fprintf(stderr, "epoll error\n");
            close(currentEvent.data.fd);
            continue;
        }

        for (const unique_ptr<RunServers> &server : servers)
        {
            int clientFD = currentEvent.data.fd;
            if (server->_listener == clientFD)
            {
                acceptConnection(server, fds);
            }
            else
            {
                processClientRequest(server, fds, clientFD);
            }
        }
    }
}

void RunServers::acceptConnection(const unique_ptr<RunServers> &server, FileDescriptor &fds)
{
    while (true)
    {
        socklen_t in_len = sizeof(struct sockaddr);
        struct sockaddr in_addr;
        int infd = accept(server->_listener, &in_addr, &in_len);
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
                "(host=%s, port=%s)\n", server->_serverName.c_str(), infd, hbuf, sbuf);
        }

        if(make_socket_non_blocking(infd) == -1)
            abort();
        
        struct epoll_event  current_event;
        current_event.data.fd = infd;
        current_event.events = EPOLLIN /* | EPOLLET */; // EPOLLET niet gebruiken, stopt meerdere pakketen verzende
        if(epoll_ctl(_epfd, EPOLL_CTL_ADD, infd, &current_event) == -1)
        {
            perror("epoll_ctl");
            abort();
        }
        fds.setFD(infd);
    }
}

void RunServers::processClientRequest(const unique_ptr<RunServers> &server, FileDescriptor& fds, int clientFD)
{
    char	buff[CLIENT_BUFFER_SIZE];
    ssize_t bytesReceived = recv(clientFD, buff, sizeof(buff), 0);

    if (bytesReceived < 0)
    {
        cleanupClient(clientFD, fds);
        if(errno == EINTR)
        {
            //	STOP SERVER CLEAN UP
        }
        if (errno != EAGAIN)
            cerr << "recv: " << strerror(errno);
        return;
    }
    size_t receivedBytes = static_cast<size_t>(bytesReceived);

    _fdBuffers[clientFD].append(buff, receivedBytes);

    ClientRequestState &state = _clientStates[clientFD];

    if (state.headerParsed == false)
    {
        size_t headerEnd = _fdBuffers[clientFD].find("\r\n\r\n");
        if (headerEnd == string::npos)
        {
            sendErrorResponse(clientFD, "400 Bad Request");
            cleanupClient(clientFD, fds);
            return;
        }

        state.header = _fdBuffers[clientFD].substr(0, headerEnd + 4);
        state.body = _fdBuffers[clientFD].substr(headerEnd + 4);
        state.headerParsed = true;

        // Parse Content-Length if POST
        state.method = extractMethod(state.header);
        if (state.method.empty() == true)
        {
            sendErrorResponse(clientFD, "400 Bad Request");
            cleanupClient(clientFD, fds);
            return;
        }
        if (state.method == "POST") {
            string contentLengthStr = extractHeader(state.header, "Content-Length:");
            if (!contentLengthStr.empty())
            {
                state.contentLength = stoll(contentLengthStr);
                if (state.contentLength <= 0)
                {
                    sendErrorResponse(clientFD, "411 Length Required");
                    _fdBuffers[clientFD].clear();
                    return;
                }
                if (state.body.size() < state.contentLength)
                    return;
            }
        }
    } else {
        // Only append new data to body
        state.body.append(buff, bytesReceived);
    }
    
    if (state.method == "POST" && state.body.size() < state.contentLength)
        return; // Wait for more data

    try
    {
        HttpRequest request(clientFD, state.method, state.header, state.body);
        request.handleRequest();
        // send(clientFD, msgToClient.c_str(), msgToClient.size(), 0);
        string ok = "HTTP/1.1 200 OK\r\n";
        send(clientFD, ok.c_str(), ok.size(), 0);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        string msgToClient = "HTTP/1.1 400 Bad Request, <html><body><h1>400 Bad Request</h1></body></html>";
        // send(clientFD, msgToClient.c_str(), msgToClient.size(), 0);
    }
    
    cleanupClient(clientFD, fds);
    printf("%s: Closed connection on descriptor %d\n", server->_serverName.c_str(), clientFD);
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

void sendErrorResponse(int clientFD, const std::string &message)
{
    std::string response = "HTTP/1.1 " + message + "\r\nContent-Length: 0\r\n\r\n";
    send(clientFD, response.c_str(), response.size(), 0);
}

void RunServers::cleanupClient(int clientFD, FileDescriptor &fds)
{
    _clientStates.erase(clientFD);
    _fdBuffers[clientFD].clear();
    if (epoll_ctl(_epfd, EPOLL_CTL_DEL, clientFD, NULL) == -1)
        perror("epoll_ctl: EPOLL_CTL_DEL");
    fds.closeFD(clientFD);
}
