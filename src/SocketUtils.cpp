#ifdef __linux__
# include <sys/epoll.h>
#endif
#include <RunServer.hpp>
#include "Logger.hpp"
#include <fcntl.h>
#include <ErrorCodeClientException.hpp>

int RunServers::epollInit(ServerList &servers)
{
    _epfd = epoll_create(FD_LIMIT); // parameter must be bigger than 0, rtfm
    if (_epfd == -1)
    {
        Logger::log(ERROR, "Server epoll_create: ", strerror(errno));
        return -1;
    }
    FileDescriptor::setFD(_epfd);
    
	map<pair<const string, string>, int> listenersMade;
    int fd;
    for (auto &server : servers)
    {
        for (pair<const string, string> &hostPort : server->getPortHost())
        {
            auto it = listenersMade.find(hostPort);
            if (it == listenersMade.end())
            {
                ServerListenFD listenerFD(hostPort.first.c_str(), hostPort.second.c_str());
                fd = listenerFD.getFD();
                FileDescriptor::setFD(fd);
                listenersMade.insert({hostPort, fd});
                _listenFDS.push_back(fd);
                RunServers::setEpollEvents(fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLET);
            }
        }
    }
    // vector<int> seenInts;
    // for (const unique_ptr<Server> &server : _servers)
    // {
    //     struct epoll_event current_event;
    //     current_event.events = EPOLLIN | EPOLLET;
    //     for (int listener : server->getListeners())
    //     {
    //         current_event.data.fd = listener;
    //         if (find(seenInts.begin(), seenInts.end(), listener) != seenInts.end())
    //             continue ;
    //         if (epoll_ctl(_epfd, EPOLL_CTL_ADD, listener, &current_event) == -1)
    //         {
    //             cerr << "Server epoll_ctl: " << strerror(errno) << endl;
    //             close(_epfd);
    //             return -1;
    //         }
    //         seenInts.push_back(listener);
    //     }
    // }
    return 0;
}

static string NumIpToString2(uint32_t addr)
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
        // throw ErrorCodeClientException(client, 500, "Failed to get server info");
    }
    
    string serverIP = NumIpToString2(ntohl(serverAddr.sin_addr.s_addr));
    uint16_t serverPort = ntohs(serverAddr.sin_port);
    string portStr = to_string(serverPort);
    
    
    // Find the matching server
    for (unique_ptr<Server> &server : _servers) {
        for (pair<const string, string> &porthost : server->getPortHost()) {
            if (porthost.first == portStr && 
                (porthost.second == serverIP || porthost.second == "0.0.0.0"))
            {
                client._usedServer = make_unique<Server>(*server); // this needs to change to use HOST as well for picking server to use
                return;
            }
        }
    }
    
    // throw ErrorCodeClientException(client, 500, "No matching server configuration found");
}

void RunServers::acceptConnection(const int listener)
{
    while (true)
    {
        socklen_t in_len = sizeof(struct sockaddr);
        struct sockaddr in_addr;
        errno = 0;
        int infd = accept(listener, &in_addr, &in_len);
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
            FileDescriptor::closeFD(infd);
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

bool RunServers::make_socket_non_blocking(int sfd)
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
        Logger::log(ERROR, "epoll_ctl: ", strerror(errno));
        // throw something
    }
}
