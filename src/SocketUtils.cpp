#ifdef __linux__
# include <sys/epoll.h>
#endif
#include <RunServer.hpp>
#include <fcntl.h>

int RunServers::epollInit(/* ServerList &servers */)
{
    _epfd = epoll_create(FD_LIMIT); // parameter must be bigger than 0, rtfm
    if (_epfd == -1)
{
        cerr << "Server epoll_create: " << strerror(errno);
        return -1;
    }
    FileDescriptor::setFD(_epfd);
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
                perror("accept");
            break;
        }
        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
        if(getnameinfo(&in_addr, in_len, hbuf, sizeof(hbuf), sbuf, 
            sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
        {
            printf("%s: Accepted connection on descriptor %d"
                "(host=%s, port=%s)\n", "server->getServerName().c_str()", infd, hbuf, sbuf);
        }
        // if(make_socket_non_blocking(infd) == -1)
        // {
        //     close(infd);
        //     break;
        // }
        struct epoll_event  current_event;
        current_event.data.fd = infd;
        current_event.events = EPOLLIN /* | EPOLLET */; // EPOLLET niet gebruiken, stopt meerdere pakketen verzende
        if(epoll_ctl(_epfd, EPOLL_CTL_ADD, infd, &current_event) == -1)
        {
            cerr << "epoll_ctl: " << strerror(errno) << endl;
            close(infd);
            break;
        }
        FileDescriptor::setFD(infd);
        // insertClientFD(infd);
        _clients[infd] = std::make_unique<Client>(infd);
		_clients[infd]->setDisconnectTime(disconnectDelaySeconds);
    }
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

void RunServers::setEpollEvents(int fd, int option, uint32_t events)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    if (epoll_ctl(_epfd, option, fd, &ev) == -1)
        std::cerr << "epoll_ctl: " << strerror(errno) << std::endl;
    // setEpollEvents(clientFD, EPOLL_CTL_MOD, EPOLLIN | EPOLLOUT);
}
