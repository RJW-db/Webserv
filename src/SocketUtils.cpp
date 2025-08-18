#ifdef __linux__
# include <sys/epoll.h>
#endif
#include <RunServer.hpp>
#include "Logger.hpp"
#include <fcntl.h>
#include <ErrorCodeClientException.hpp>

void RunServers::epollInit(ServerList &servers)
{
    _epfd = epoll_create(FD_LIMIT); // parameter must be bigger than 0, rtfm
    if (_epfd == -1)
        Logger::logExit(ERROR, "Setup failed: Server epoll_create: ", strerror(errno));
    FileDescriptor::setFD(_epfd);
    Logger::log(INFO, "Epoll fd created       epollFD:", _epfd);

	map<pair<const string, string>, int> listenersMade;
    for (auto &server : servers)
    {
        for (pair<const string, string> &hostPort : server->getPortHost())
        {
            auto it = listenersMade.find(hostPort);
            if (it == listenersMade.end())
            {
                ServerListenFD listenerFD(hostPort.first.c_str(), hostPort.second.c_str());
                listenersMade.insert({hostPort, listenerFD.getFD()});
                _listenFDS.push_back(listenerFD.getFD());
            }
        }
    }
}

static string NumIpToString(uint32_t addr)
{
    unsigned char bytes[4];
    bytes[0] = (addr >> 24) & 0xFF;
    bytes[1] = (addr >> 16) & 0xFF;
    bytes[2] = (addr >> 8) & 0xFF;
    bytes[3] = addr & 0xFF;

    return to_string(bytes[0]) + "." +
           to_string(bytes[1]) + "." +
           to_string(bytes[2]) + "." +
           to_string(bytes[3]);
}


void RunServers::setServerFromListener(Client &client)
{
    auto hostHeader = client._headerFields.find("Host");
    if (hostHeader == client._headerFields.end()) {
        throw ErrorCodeClientException(client, 400, "Missing required Host header");
    }
    Server *tmpServer = nullptr;
    // Find the matching server
    for (unique_ptr<Server> &server : _servers) {
        for (pair<const string, string> &porthost : server->getPortHost()) {
            if (porthost.first == client._ipPort.second && 
                (porthost.second == client._ipPort.first || porthost.second == "0.0.0.0"))
            {
                if (server->getServerName() == hostHeader->second)
                {
                    client._usedServer = make_unique<Server>(*server);
                    return;
                }
                if (tmpServer == nullptr)
                    tmpServer = server.get();
            }
        }
    }
    if (tmpServer != nullptr)
        client._usedServer = make_unique<Server>(*tmpServer);
    else
        throw ErrorCodeClientException(client, 0, "No matching server configuration found");
}

void RunServers::acceptConnection(const int listener)
{
    while (true)
    {
        socklen_t in_len = sizeof(struct sockaddr);
        struct sockaddr in_addr;
        int infd = accept(listener, &in_addr, &in_len);
        if (infd == -1)
        {
            if (errno != EAGAIN)
                Logger::log(ERROR, "Server accept: ", strerror(errno));
            break;
        }
        struct epoll_event  current_event;
        current_event.data.fd = infd;
        current_event.events = EPOLLIN;
        if (epoll_ctl(_epfd, EPOLL_CTL_ADD, infd, &current_event) == -1)
        {
            Logger::log(ERROR, "epoll_ctl: ", strerror(errno));
            if (FileDescriptor::safeCloseFD(infd) == false)
                Logger::logExit(FATAL, "FileDescriptor::safeCloseFD: Attempted to close a file descriptor that is not in the vector: ", infd);
            break;
        }
        FileDescriptor::setFD(infd);
        
        _clients[infd] = std::make_unique<Client>(infd);
        // setServerFromListener(*_clients[infd], listener);

        sockaddr_in serverAddr;
        socklen_t addrLen = sizeof(serverAddr); 
        if (getsockname(infd, (struct sockaddr*)&serverAddr, &addrLen) != 0)
            throw ErrorCodeClientException(*_clients[infd], 500, "Failed to get server info"); //TODO not protected
        throwTesting();
        _clients[infd]->_ipPort.first = NumIpToString(ntohl(serverAddr.sin_addr.s_addr));
        _clients[infd]->_ipPort.second = to_string(ntohs(serverAddr.sin_port));

        Logger::log(INFO, *_clients[infd], "Connected on: ",
            NumIpToString(ntohl(((sockaddr_in *)&in_addr)->sin_addr.s_addr)),
            ":", ntohs(((sockaddr_in *)&in_addr)->sin_port));
        _clients[infd]->setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    }
}

bool RunServers::makeSocketNonBlocking(int sfd)
{
    int currentFlags = fcntl(sfd, F_GETFL, 0);
    if (currentFlags == -1)
    {
        Logger::log(ERROR, "fcntl: ", strerror(errno));
        return false;
    }

    currentFlags |= O_NONBLOCK;
    int fcntlResult = fcntl(sfd, F_SETFL, currentFlags);
    if (fcntlResult == -1)
    {
        Logger::log(ERROR, "fcntl: ", strerror(errno));
        return false;
    }
    return true;
}

// setEpollEvents(clientFD, EPOLL_CTL_MOD, EPOLLIN | EPOLLOUT);
void RunServers::setEpollEvents(int fd, int option, uint32_t events)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    if (epoll_ctl(_epfd, option, fd, &ev) == -1)
    {
        if (_clients.count(fd) == 0 || !_clients[fd])
        {
            Logger::log(ERROR, "setEpollEvents: invalid client FD ", fd);
            throw std::runtime_error("epoll_ctl failed: " + string(strerror(errno)));
        }
        throw ErrorCodeClientException(*_clients[fd], 0, "epoll_ctl failed: " + string(strerror(errno)) + " for fd: " + to_string(fd));
    }
}
