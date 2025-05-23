#include <Webserv.hpp>
#include <iostream>
#include <FileDescriptor.hpp>

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

int Server::_epfd = -1;
std::array<struct epoll_event, FD_LIMIT> Server::_events;

std::unordered_map<int, std::string> Server::_fdBuffers;

Server::Server(tmp_t *serverConf)
{
    ServerListenFD listenerFD(serverConf->port);
    _serverName = serverConf->hostname;
    _listener = listenerFD.getFD();
}


Server::~Server()
{
    close(_listener);
    close(_epfd);
}

bool Server::directoryCheck(string &path)
{
    DIR *d = opendir("/home");	// path = rde-brui
    if (d == NULL) {
        perror("opendir");
        return false;
    }

    struct dirent *directoryEntry;
    while ((directoryEntry = readdir(d)) != NULL) {
        // printf("%s\n", directoryEntry->d_name);
        if (string(directoryEntry->d_name) == path)
        {
            closedir(d);
            return (true);
        }
    }
    
    closedir(d);
    return (false);
}

int Server::make_socket_non_blocking(int sfd)
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

int Server::epollInit(ServerList &servers)
{
    _epfd = epoll_create(FD_LIMIT); // parameter must be bigger than 0, rtfm
    if (_epfd == -1)
    {
        std::cerr << "Server epoll_create: " << strerror(errno);
        return -1;
    }

    for (const std::unique_ptr<Server> &server : servers)
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

int Server::runServers(ServerList &servers, FileDescriptor &fds)
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

void Server::handleEvents(ServerList &servers, FileDescriptor &fds, size_t eventCount)
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

        for (const unique_ptr<Server> &server : servers)
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

void Server::acceptConnection(const unique_ptr<Server> &server, FileDescriptor &fds)
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
        current_event.events = EPOLLIN /* | EPOLLET */;
        if(epoll_ctl(_epfd, EPOLL_CTL_ADD, infd, &current_event) == -1)
        {
            perror("epoll_ctl");
            abort();
        }
        fds.setFD(infd);
    }
}

void Server::processClientRequest(const unique_ptr<Server> &server, FileDescriptor& fds, int clientFD)
{
    char	buff[CLIENT_BUFFER_SIZE];

    ssize_t bytesReceived = recv(clientFD, buff, sizeof(buff), 0);
    if (bytesReceived < 0)
    {
        fds.closeFD(clientFD);
        if(errno == EINTR)
        {
            //	STOP SERVER CLEAN UP
        }
        if (errno != EAGAIN)
            cerr << "recv: " << strerror(errno);
    }
    size_t receivedBytes = static_cast<size_t>(bytesReceived);
    _fdBuffers[clientFD].append(buff, receivedBytes);
    if (receivedBytes < sizeof(buff) ||
        (receivedBytes == sizeof(buff) && buff[CLIENT_BUFFER_SIZE - 1] == '\n'))
    {
        try
        {
            parseHttpRequest(_fdBuffers[clientFD]);
        }
        catch(const ClientException &e)
        {
            std::cerr << e.what() << endl;  // should use to log to file
            string msgToClient = "HTTP/1.1 400 Bad Request, <html><body><h1>400 Bad Request</h1></body></html>";
            send(clientFD, msgToClient.c_str(), msgToClient.size(), 0);
            _fdBuffers[clientFD].clear();
            if (epoll_ctl(_epfd, EPOLL_CTL_DEL, clientFD, NULL) == -1)
                perror("epoll_ctl: EPOLL_CTL_DEL");
            printf("%s: Closed connection on descriptor %d\n", server->_serverName.c_str(), clientFD);
            return ;
        }
        // parseHttpRequest(_fdBuffers[clientFD]);
        send(clientFD, _fdBuffers[clientFD].c_str(), _fdBuffers[clientFD].size(), 0);
        // std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        // send(clientFD, response.c_str(), response.size(), 0);
        // write(1, _fdBuffers[clientFD].c_str(), _fdBuffers[clientFD].size());
        _fdBuffers[clientFD].clear();
        if (epoll_ctl(_epfd, EPOLL_CTL_DEL, clientFD, NULL) == -1)
            perror("epoll_ctl: EPOLL_CTL_DEL");
        printf("%s: Closed connection on descriptor %d\n", server->_serverName.c_str(), clientFD);
        fds.closeFD(clientFD);
    }
}
