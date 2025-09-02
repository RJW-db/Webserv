
#include <unordered_map>
#include <sys/wait.h>
#include <filesystem> // filesystem::path
#include <limits.h>   // PATH_MAX
#ifdef __linux__
# include <sys/epoll.h>
#endif
#include "ErrorCodeClientException.hpp"
#include "FileDescriptor.hpp"
#include "ServerListenFD.hpp"
#include "ConfigServer.hpp"
#include "RunServer.hpp"
#include "Logger.hpp"
namespace
{
    constexpr const char   *EPOLL_OPTIONS[3] = {
        "EPOLL_CTL_ADD: ",
        "EPOLL_CTL_DEL: ",
        "EPOLL_CTL_MOD: "
    };
}

void RunServers::getExecutableDirectory()
{
    try
    {
        char buf[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len == -1)
            Logger::logExit(ERROR, "Server error", '-', "readlink failed: ", strerror(errno));
        buf[len] = '\0';

        filesystem::path exePath(buf);
        _serverRootDir = exePath.parent_path().string();
    }
    catch (const exception& e)
    {
        Logger::logExit(ERROR, "Server error", '-', "Cannot determine executable directory: " + string(e.what()));
    }
}

void RunServers::createServers(vector<ConfigServer> &configs)
{
    for (ConfigServer &config : configs)
        _servers.push_back(make_unique<AconfigServ>(AconfigServ(config)));
}

void RunServers::setupEpoll()
{
    epollInit(_servers);
    // if (TERMINAL_DEBUG)
    //     addStdinToEpoll();
}

void RunServers::epollInit(ServerList &servers)
{
    _epfd = epoll_create(FD_LIMIT); // parameter must be bigger than 0, rtfm
    if (_epfd == -1)
        Logger::logExit(ERROR, "Server error", "-", "Server epoll_create: ", strerror(errno));
    FileDescriptor::setFD(_epfd);
    Logger::log(INFO, "Epoll fd created", _epfd, "epollFD");

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

void RunServers::addStdinToEpoll()
{
    int stdin_fd = 0;

    if (FileDescriptor::setNonBlocking(stdin_fd) == false)
        Logger::logExit(ERROR, "Server error" , '-', "Failed to set stdin to non-blocking mode");

    struct epoll_event current_event;
    current_event.data.fd = stdin_fd;
    current_event.events = EPOLLIN;
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, stdin_fd, &current_event) == -1)
        Logger::logExit(ERROR, "Server error" , '-', "epoll_ctl (stdin): ", strerror(errno));
}

// setEpollEvents(clientFD, EPOLL_CTL_MOD, EPOLLIN | EPOLLOUT);
void RunServers::setEpollEvents(int fd, int option, uint32_t events)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    if (epoll_ctl(_epfd, option, fd, &ev) == -1)
        Logger::logExit(FATAL, "Server Error", fd, "Invalid FD, setEpollEvents failed");
    if (option == EPOLL_CTL_ADD)
        _epollAddedFds.push_back(fd);
}

// setEpollEventsClient(clientFD, EPOLL_CTL_MOD, EPOLLIN | EPOLLOUT);
void RunServers::setEpollEventsClient(Client &client, int fd, int option, uint32_t events)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    // throwTesting();
    // static int testCount = 0;
    if (option == EPOLL_CTL_DEL || epoll_ctl(_epfd, option, fd, &ev) == -1)
    {
        if (_clients.count(fd) == 0) // pipes
        {
            if (option == EPOLL_CTL_DEL)
                Logger::logExit(IERROR, "epoll_ctl error", fd, "fd", EPOLL_OPTIONS[option - 1], strerror(errno));
            throw ErrorCodeClientException(client, 502, "Server Error Pipes: " + string(EPOLL_OPTIONS[option - 1]) + "fd:" + to_string(fd) + " Failed: " + strerror(errno));
        }
        else // client
        {
            if (option == EPOLL_CTL_DEL)
                Logger::logExit(IERROR, "epoll_ctl error", fd, "fd", EPOLL_OPTIONS[option - 1], strerror(errno));
            if (events & EPOLLOUT || option == EPOLL_CTL_ADD)
                throw ErrorCodeClientException(client, 0, "Server Error Client: " + string(EPOLL_OPTIONS[option - 1]) + "fd:" + to_string(fd) + " Failed: " + strerror(errno));
            if (events & EPOLLIN)
                throw ErrorCodeClientException(client, 500, "Server Error Client: " + string(EPOLL_OPTIONS[option - 1]) + "fd:" + to_string(fd) + " Failed: " + strerror(errno));
        }
    }
    if (option == EPOLL_CTL_ADD)
        _epollAddedFds.push_back(fd);
}

void RunServers::setServerFromListener(Client &client)
{
    auto hostHeader = client._headerFields.find("Host");
    if (hostHeader == client._headerFields.end()) {
        throw ErrorCodeClientException(client, 400, "Missing required Host header");
    }
    AconfigServ *tmpServer = nullptr;
    // Find the matching server
    for (unique_ptr<AconfigServ> &server : _servers) {
        for (pair<const string, string> &porthost : server->getPortHost()) {
            if (porthost.first == client._ipPort.second && 
                (porthost.second == client._ipPort.first || porthost.second == "0.0.0.0"))
            {
                if (server->getServerName() == hostHeader->second)
                {
                    client._usedServer = make_unique<AconfigServ>(*server);
                    return;
                }
                if (tmpServer == nullptr)
                    tmpServer = server.get();
            }
        }
    }
    if (tmpServer != nullptr)
        client._usedServer = make_unique<AconfigServ>(*tmpServer);
    else
        throw ErrorCodeClientException(client, 0, "No matching server configuration found");
}

void    RunServers::setLocation(Client &client)
{
    for (pair<string, Location> &locationPair : client._usedServer->getLocations())
    {
        if (strncmp(client._requestPath.data(), locationPair.first.data(), locationPair.first.size()) == 0 &&
        (client._requestPath[client._requestPath.size()] == '\0' || client._requestPath[locationPair.first.size() - 1] == '/'))
        {
            client._location = locationPair.second;
            return;
        }
    }
    throw ErrorCodeClientException(client, 400, "Couldn't find location block: malformed request");
}

void RunServers::cleanupEpoll()
{
    for (auto it = _listenFDS.begin(); it != _listenFDS.end();)
    {
        FileDescriptor::cleanupEpollFd(*it);
        it = _listenFDS.erase(it);
    }
    while (_clients.size() > 0)
    {
        cleanupClient(*_clients.begin()->second);
    }
    FileDescriptor::closeFD(_epfd);
    FileDescriptor::cleanupAllFD(); // left overs like fd for files
}
