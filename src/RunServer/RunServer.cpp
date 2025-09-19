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

// --- Server configuration ---
string RunServers::_serverRootDir;
ServerList RunServers::_servers;
vector<int> RunServers::_listenFDS;
vector<int> RunServers::_epollAddedFds;

// --- Epoll and event management ---
int RunServers::_epfd = -1;
array<struct epoll_event, FD_LIMIT> RunServers::_events;

// --- Client and handle management ---
unordered_map<int, unique_ptr<Client>> RunServers::_clients;
vector<unique_ptr<HandleTransfer>> RunServers::_handle;
vector<unique_ptr<HandleTransfer>> RunServers::_handleCgi;
unordered_map<string, SessionData> RunServers::sessions;
// --- Miscellaneous ---
int RunServers::_level = -1;
uint64_t RunServers::_ramBufferLimit = 65536; // 64KB
uint8_t RunServers::_fatalErrorOccurred = SERVER_GOOD;

void RunServers::runServers()
{
    while (g_signal_status == 0) {
        int eventCount;
        
        if (_fatalErrorOccurred != SERVER_GOOD) {
            handleFatalError();
        }

        disconnectChecks();
        eventCount = epoll_wait(_epfd, _events.data(), FD_LIMIT, DISCONNECT_DELAY_SECONDS);
        if (eventCount == -1) {
            if (errno == EINTR)
            {
                if (g_signal_status != SIGINT)
                    Logger::log(WARN, "epoll_wait interrupted", "-", "by signal: ", g_signal_status);
                continue;
            }
            Logger::logExit(ERROR, "Server error", '-', "Server epoll_wait: ", strerror(errno));
        }
        try {
            handleEvents(static_cast<size_t>(eventCount));
        }
        catch (Logger::ErrorLogExit&) {
            Logger::logExit(ERROR, "Server error", '-', "Fatal error occurred, shutting down now");
        }
        catch (const exception& e) {
            Logger::log(ERROR, "Server error", '-', "Exception in handleEvents: ", e.what());
        }
    }
    Logger::log(INFO, "Server shutting down gracefully...");
}

void RunServers::handleEvents(size_t eventCount)
{
    for (size_t i = 0; i < eventCount; ++i) {
        try {
            struct epoll_event &currentEvent = _events[i];
            int eventFD = currentEvent.data.fd;

            if (eventFD == 0 && (currentEvent.events & EPOLLIN)) {
                handleEpollStdinEvents();
                continue;
            }

            if (handleEpollErrorEvents(currentEvent, eventFD))
                continue;

            if (find(_listenFDS.begin(), _listenFDS.end(), eventFD) != _listenFDS.end() &&
                _fatalErrorOccurred == SERVER_GOOD)
                acceptConnection(eventFD);

            if (runHandleTransfer(currentEvent) == true || \
                runCgiHandleTransfer(currentEvent) == true)
                continue;

            if ((_clients.find(eventFD) != _clients.end()) &&
                (currentEvent.events == EPOLLIN))
                processClientRequest(*_clients[eventFD].get());
        }
        catch (ErrorCodeClientException &e) {
            e.handleErrorClient();
        }
    }
}

void RunServers::handleEpollStdinEvents()
{
    char buffer[KILOBYTE];
    ssize_t bytesRead = read(0, buffer, sizeof(buffer) - 1);
    if (bytesRead == -1)
        Logger::log(IWARN, "Reading from stdin failed: ", strerror(errno));
    else if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        int snooze = stoi(buffer, nullptr, 10);
        if (snooze > 0 && snooze < 20)
            this_thread::sleep_for(chrono::seconds(snooze));
    }
}

bool RunServers::handleEpollErrorEvents(const struct epoll_event &currentEvent, int eventFD)
{
    if ((currentEvent.events & (EPOLLERR | EPOLLHUP)) ||
        !(currentEvent.events & (EPOLLIN | EPOLLOUT))) {
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
            removeHandlesWithFD(eventFD);
        return true;
    }
    return false;
}

bool RunServers::runHandleTransfer(struct epoll_event &currentEvent)
{
    int eventFD = currentEvent.data.fd;
    for (size_t idx = 0; idx < _handle.size(); ++idx) {
        if (_handle[idx]->_client._fd == eventFD) {
            HandleTransfer &handle = *_handle[idx];
            if (processHandleTransferEvents(currentEvent, handle)) {
                handleCompletedTransfer(currentEvent, handle, idx);
            }
            return true;
        }
    }
    return false;
}

bool RunServers::processHandleTransferEvents(const struct epoll_event& currentEvent, HandleTransfer& handle)
{
    if (currentEvent.events & EPOLLOUT) {
        if (handle._handleType == HANDLE_GET_TRANSFER)
            return handle.handleGetTransfer();
        else
            return handle.sendToClientTransfer();
    }
    else if (currentEvent.events & EPOLLIN) {
        if (handle._handleType == HANDLE_POST_TRANSFER)
            return handle.postTransfer(true);
        else {
            handle.appendToBody();
            return handle.handleChunkTransfer();
        }
    }
    return false;
}

void RunServers::handleCompletedTransfer(const struct epoll_event& currentEvent, HandleTransfer& handle, size_t idx)
{
    if (currentEvent.events & EPOLLOUT) {
        Client& client = handle._client;
        if (client._keepAlive)
            client.httpCleanup();
        else
        {
            cleanupClient(client);
            return;
        }
    }
    _handle.erase(_handle.begin() + static_cast<long>(idx));

}

bool RunServers::runCgiHandleTransfer(struct epoll_event &currentEvent)
{
    int eventFD = currentEvent.data.fd;
    for (auto it = _handleCgi.begin(); it != _handleCgi.end(); ++it) {
        if ((*it)->_fd == eventFD) {
            if (currentEvent.events & EPOLLOUT) {
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

void RunServers::handleFatalError()
{
    if (_fatalErrorOccurred == FATAL_ERROR_CLOSE_LISTENERS) {
        Logger::log(ERROR, "Server error", '-', "Fatal error occurred, closing listeners and stopping new connections");
        
        for (int listener : _listenFDS)
            FileDescriptor::cleanupEpollFd(listener);
        _listenFDS.clear();
        
        _fatalErrorOccurred = FATAL_ERROR_HANDLE_CLIENTS;
    
        for (const pair<const int, unique_ptr<Client>> &clientPair : _clients) {
            clientPair.second->_keepAlive = false;
        }
    }
    if (_fatalErrorOccurred == FATAL_ERROR_HANDLE_CLIENTS &&
        _handle.empty() && _handleCgi.empty()) {
        Logger::logExit(FATAL, "Server error", '-', "Fatal error occurred, handled all clients and exiting now");
    }
}
