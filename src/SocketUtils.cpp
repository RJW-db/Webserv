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
    Logger::log(INFO, "Epoll fd created       FD:", _epfd);

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
    uint32_t num;
    string result;
    int bitshift = 24;
    uint32_t rev_addr = htonl(addr);

    for (uint8_t i = 0; i < 4; ++i)
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

void RunServers::setServerFromListener(Client &client, int listenerFD)
{
    // Get the server info from the listener socket
    sockaddr_in serverAddr;
    socklen_t addrLen = sizeof(serverAddr);
    if (getsockname(listenerFD, (struct sockaddr*)&serverAddr, &addrLen) != 0) {
        throw ErrorCodeClientException(client, 500, "Failed to get server info"); //TODO not protected
    }
    
    string serverIP = NumIpToString(ntohl(serverAddr.sin_addr.s_addr));
    uint16_t serverPort = ntohs(serverAddr.sin_port);
    string portStr = to_string(serverPort);
    // throw bad_alloc(); // TODO: this line leaks
    // Find the matching server
    for (unique_ptr<Server> &server : _servers) {
        for (pair<const string, string> &porthost : server->getPortHost()) {
            if (porthost.first == portStr && 
                (porthost.second == serverIP || porthost.second == "0.0.0.0"))
            {
                client._usedServer = make_unique<Server>(*server);
                return;
            }
        }
    }
    throw ErrorCodeClientException(client, 500, "No matching server configuration found");
}

void RunServers::acceptConnection(const int listener)
{
    while (true)
    {
        socklen_t in_len = sizeof(struct sockaddr);
        struct sockaddr in_addr;
        errno = 0;
        int infd = accept(listener, &in_addr, &in_len); // TODO does it matter if server accepts on different listener fd than what it was caught on?
        if(infd == -1)
        {
            if(errno != EAGAIN)
                Logger::log(ERROR, "Server accept: ", strerror(errno));
            break;
        }
        struct epoll_event  current_event;
        current_event.data.fd = infd;
        current_event.events = EPOLLIN /* | EPOLLET */; // EPOLLET niet gebruiken, stopt meerdere pakketen verzende
        if(epoll_ctl(_epfd, EPOLL_CTL_ADD, infd, &current_event) == -1)
        {
            Logger::log(ERROR, "epoll_ctl: ", strerror(errno));
            if (FileDescriptor::safeCloseFD(infd) == false)
                Logger::logExit(FATAL, "FileDescriptor::safeCloseFD: Attempted to close a file descriptor that is not in the vector: ", infd);
            break;
        }
        FileDescriptor::setFD(infd);
        // insertClientFD(infd);
        _clients[infd] = std::make_unique<Client>(infd);
		_clients[infd]->setDisconnectTime(DISCONNECT_DELAY_SECONDS);
        // RunServers::setServer(*_clients[infd]);
        setServerFromListener(*_clients[infd], listener);
        Logger::log(INFO, *_clients[infd], "Connected");
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
    if (/* fd == 7 ||  */epoll_ctl(_epfd, option, fd, &ev) == -1)
    {
        if (_clients.count(fd) == 0 || !_clients[fd])
        {
            Logger::log(ERROR, "setEpollEvents: invalid client FD ", fd);
            throw std::runtime_error("epoll_ctl failed: " + string(strerror(errno)));
        }
        throw ErrorCodeClientException(*_clients[fd], 500, "epoll_ctl failed: " + string(strerror(errno)) + " for fd: " + to_string(fd));
    }
}
