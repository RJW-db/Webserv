#include <sys/wait.h>
#include <algorithm>
#include <signal.h>
#include <thread>
#include <array>
#ifdef __linux__
# include <sys/epoll.h>
#endif
#include "ErrorCodeClientException.hpp"
#include "FileDescriptor.hpp"
#include "HandleTransfer.hpp"
#include "HttpRequest.hpp"
#include "RunServer.hpp"
#include "Constants.hpp"
#include "Logger.hpp"

// Static member variables
// FileDescriptor RunServers::_fds;
string RunServers::_serverRootDir;
int RunServers::_epfd = -1;
array<struct epoll_event, FD_LIMIT> RunServers::_events;
// unordered_map<int, string> RunServers::_fdBuffers;
ServerList RunServers::_servers;
vector<int> RunServers::_listenFDS;
vector<int> RunServers::_epollAddedFds;
vector<unique_ptr<HandleTransfer>> RunServers::_handle;
vector<unique_ptr<HandleTransfer>> RunServers::_handleCgi;
// vector<int> RunServers::_connectedClients;
unordered_map<int, unique_ptr<Client>> RunServers::_clients;
int RunServers::_level = -1;

uint64_t RunServers::_ramBufferLimit = 65536;
bool RunServers::_fatalErrorOccurred = false;


void RunServers::runServers()
{
    while (g_signal_status == 0)
    {
        int eventCount;

        disconnectChecks();
        if (g_signal_status == 0)
            break;
        eventCount = epoll_wait(_epfd, _events.data(), FD_LIMIT, DISCONNECT_DELAY_SECONDS);

        if (eventCount == -1)
        {
            if (errno == EINTR)
            {
                Logger::log(WARN, "epoll_wait interrupted by signal: ", g_signal_status);
                continue;
            }
            Logger::logExit(ERROR, "Server error", '-', "Server epoll_wait: ", strerror(errno));
        }
        try
        {
            // cout << "event count "<<  eventCount << endl;
            handleEvents(static_cast<size_t>(eventCount));
        }
        catch (Logger::ErrorLogExit&)
        {
            Logger::logExit(ERROR, "Server error", '-', "Restart now or finish existing clients and exit");
        }
        catch (const exception& e)
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
                if (handle._handleType == HANDLE_POST_TRANSFER)
                    finished = handle.postTransfer(true);
                else
                {
                    handle.appendToBody();
                    finished = handle.handleChunkTransfer();
                }
            }
            if (finished == true)
            {
                if ( currentEvent.events & EPOLLOUT) // has to be send to client for cleanup
                {
                    if (client._keepAlive == false)
                    {
                        cleanupClient(client);
                        return true;
                    }
                    else
                        clientHttpCleanup(client);
                }
                _handle.erase(_handle.begin() + static_cast<long>(idx));
            }
            return true;
        }
    }
    return false;
}

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
                {
                    _handleCgi.erase(it);
                }
            }
            else if (currentEvent.events & EPOLLIN)
                (*it)->readFromCgiTransfer();
            return true;
        }
    }
    return false;
}
