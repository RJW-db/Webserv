// #include <chrono>
// #include <ctime>
// #include <iostream>
// #include <arpa/inet.h>
// #include <cstring>
// #include <errno.h>
// #include <fcntl.h>
// #include <netdb.h>
// #include <netinet/in.h>
// #include <stdio.h>
// #include <sys/socket.h>
// #include <sys/types.h>
// #include <stdlib.h>	// callod
// #include <filesystem> // canonical()
#include <thread>
#ifdef __linux__
# include <sys/epoll.h>
#endif
#include <signal.h>
#include <sys/wait.h>
#include <array>

#include "RunServer.hpp"
#include "ErrorCodeClientException.hpp"
#include "HttpRequest.hpp"
#include "FileDescriptor.hpp"
#include "HandleTransfer.hpp"
#include "Logger.hpp"

// Static member variables
// FileDescriptor RunServers::_fds;
string RunServers::_serverRootDir;
int RunServers::_epfd = -1;
array<struct epoll_event, FD_LIMIT> RunServers::_events;
// unordered_map<int, string> RunServers::_fdBuffers;
ServerList RunServers::_servers;
vector<int> RunServers::_listenFDS;
vector<unique_ptr<HandleTransfer>> RunServers::_handle;
vector<unique_ptr<HandleTransfer>> RunServers::_handleCgi;
// vector<int> RunServers::_connectedClients;
unordered_map<int, unique_ptr<Client>> RunServers::_clients;
int RunServers::_level = -1;

uint64_t RunServers::_ramBufferLimit = 65536;

void RunServers::runServers()
{
    while (g_signal_status == 0)
    {
        int eventCount;

        disconnectChecks();
        eventCount = epoll_wait(_epfd, _events.data(), FD_LIMIT, DISCONNECT_DELAY_SECONDS);

        // only goes wrong with EINTR(signals)
        if (eventCount == -1)
        {
            if (errno == EINTR)
                break ;
            Logger::logExit(ERROR, "Server error", '-', "Server epoll_wait: ", strerror(errno));
        }
        try
        {
            // cout << "event count "<<  eventCount << endl;
            handleEvents(static_cast<size_t>(eventCount));
        }
        catch(Logger::ErrorLogExit&)
        {
            Logger::logExit(ERROR, "Server error", '-', "Restart now or finish existing clients and exit");
        }
        catch(const exception& e)
        {
            Logger::log(ERROR, "Server error", '-', "Exception in handleEvents: ", e.what());
        }
    }
    Logger::log(INFO, "Server shutting down gracefully...");
}

void RunServers::handleEvents(size_t eventCount)
{
    for (size_t i = 0; i < eventCount; ++i)
    {
        try
        {
            struct epoll_event &currentEvent = _events[i];
            int eventFD = currentEvent.data.fd;

            if (eventFD == 0 && (currentEvent.events & EPOLLIN) &&
                handleEpollStdinEvents())
                continue;

            if (handleEpollErrorEvents(currentEvent, eventFD))
                continue;

            if (find(_listenFDS.begin(), _listenFDS.end(), eventFD) != _listenFDS.end())
                acceptConnection(eventFD);

            if (runHandleTransfer(currentEvent) == true || \
                runCgiHandleTransfer(currentEvent) == true)
                continue;

            if ((_clients.find(eventFD) != _clients.end()) &&
                (currentEvent.events == EPOLLIN))
                processClientRequest(*_clients[eventFD].get());
        }
        catch (const ErrorCodeClientException &e)
        {
            e.handleErrorClient();
        }
    }
}

bool RunServers::handleEpollStdinEvents()
{
    char buffer[1024];
    ssize_t bytesRead = read(0, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
    }
    int snooze = stoi(buffer, nullptr, 16); //TODO not protectect
    if (snooze > 0 && snooze < 20)
    {
        this_thread::sleep_for(chrono::seconds(snooze));
    }
    return true;
}

bool RunServers::handleEpollErrorEvents(const struct epoll_event &currentEvent, int eventFD)
{
    if ((currentEvent.events & (EPOLLERR | EPOLLHUP)) ||
        !(currentEvent.events & (EPOLLIN | EPOLLOUT)))
    {
        if (currentEvent.events & EPOLLERR)
            Logger::log(INFO, "Epoll event", eventFD, "", "EPOLLERR (events: ", static_cast<int>(currentEvent.events), ')');
        if (currentEvent.events & EPOLLHUP)
            Logger::log(INFO, "Epoll event", eventFD, "", "EPOLLHUP (events: ", static_cast<int>(currentEvent.events), ')');
        if (!(currentEvent.events & (EPOLLERR | EPOLLHUP)) &&
            !(currentEvent.events & (EPOLLIN | EPOLLOUT)))
            Logger::log(IWARN, "Epoll event", eventFD, "", "Unexpected (events: ", static_cast<int>(currentEvent.events), ") - no EPOLLIN or EPOLLOUT");

        auto clientIt = _clients.find(eventFD);
        if (clientIt != _clients.end() && clientIt->second)
            cleanupClient(*clientIt->second);
        else
        {
            removeHandlesWithFD(eventFD);
            FileDescriptor::cleanupFD(eventFD);
        }
        return true;
    }
    return false;
}

bool RunServers::runHandleTransfer(struct epoll_event &currentEvent)
{
    int eventFD = currentEvent.data.fd;
    for (size_t idx = 0; idx < _handle.size(); ++idx)
    {
        if (_handle[idx]->_client._fd == eventFD)
        {
            HandleTransfer &handle = *_handle[idx];
            Client &client = handle._client;
            client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
            bool finished = false;
            if (currentEvent.events & EPOLLOUT)
            {
                if (handle._handleType == HANDLE_GET_TRANSFER)
                    finished = handle.handleGetTransfer();
                else
                    finished = handle.sendToClientTransfer();
            }
            else if (currentEvent.events & EPOLLIN)
            {
                if (handle.getIsChunk() == false)
                    finished = handle.handlePostTransfer(true);
                else
                {
                    handle.appendToBody();
                    finished = handle.handleChunkTransfer();
                }
            }
            if (finished == true)
            {
                if (client._keepAlive == false && client._isCgi == false)
                {
                    cleanupClient(client);
                }
                else
                {
                    _handle.erase(_handle.begin() + idx);
                    clientHttpCleanup(client);
                }
            }
            return true;
        }
    }
    return false;
}

// bool RunServers::runHandleTransfer(struct epoll_event &currentEvent)
// {
//     int eventFD = currentEvent.data.fd;
//     for (auto it = _handle.begin(); it != _handle.end(); ++it)
//     {
//         if ((*it)->_client._fd == eventFD)
//         {
//             HandleTransfer &handle = **it;
//             Client &client = handle._client;
//             _clients[client._fd]->setDisconnectTime(DISCONNECT_DELAY_SECONDS);
//             bool finished = false;
//             if (currentEvent.events & EPOLLOUT)
//             {
//                 if (handle._handleType == HANDLE_GET_TRANSFER)
//                     finished = handle.handleGetTransfer();
//                 else
//                     finished = handle.sendToClientTransfer();
//             }
//             else if (currentEvent.events & EPOLLIN)
//             {
//                 if ((*it)->getIsChunk() == false)
//                     finished = handle.handlePostTransfer(true);
//                 else
//                 {
//                     handle.appendToBody();
//                     finished = handle.handleChunkTransfer();
//                 }
//             }
//             if (finished == true)
//             {
//                 if (client._keepAlive == false && client._isCgi == false)
//                 {
//                     cleanupClient(*_clients[(*it)->_client._fd]);
//                 }
//                 else
//                 {
//                     _handle.erase(it);
//                     clientHttpCleanup(client);
//                 }
//             }
//             return true;
//         }
//     }
//     return false;
// }

bool RunServers::runCgiHandleTransfer(struct epoll_event &currentEvent)
{
    int eventFD = currentEvent.data.fd;
    for (auto it = _handleCgi.begin(); it != _handleCgi.end(); ++it)
    {
        if ((*it)->_fd == eventFD)
        {
            if (currentEvent.events & EPOLLOUT)
            {
                if ((*it)->writeToCgiTransfer() == true)
                    _handleCgi.erase(it);
            }
            else if (currentEvent.events & EPOLLIN)
                (*it)->readFromCgiTransfer();
            return true;
        }
    }
    return false;
}
