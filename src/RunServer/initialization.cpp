#ifdef __linux__
# include <sys/epoll.h>
#endif
#include <sys/wait.h>
#include <limits.h> // PATH_MAX
#include <filesystem> // std::filesystem::path
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
// #include <HttpRequest.hpp>
// #include <iostream>
// #include <FileDescriptor.hpp>
// #include <HandleTransfer.hpp>
#include "Logger.hpp"

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
    {
        _servers.push_back(make_unique<Server>(Server(config)));
    }
    // Server::createListeners(_servers);
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
