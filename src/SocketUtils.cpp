#ifdef __linux__
# include <sys/epoll.h>
#endif
#include <RunServer.hpp>
#include "Logger.hpp"
#include <fcntl.h>
#include <ErrorCodeClientException.hpp>


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
            Logger::log(ERROR, "Server Error", fd, "Invalid clientFD, setEpollEvents failed");
            throw std::runtime_error("epoll_ctl failed: " + string(strerror(errno)));
        }
        throw ErrorCodeClientException(*_clients[fd], 0, "epoll_ctl failed: " + string(strerror(errno)) + " for fd: " + to_string(fd));
    }
}
