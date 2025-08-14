#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>
#include <iostream>
#include <FileDescriptor.hpp>
#include <HandleTransfer.hpp>
#include "Logger.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // close
#include <stdlib.h>	// callod
#ifdef __linux__
# include <sys/epoll.h>
#endif
#include <sys/socket.h>
#include <filesystem> // canonical()

#include <ctime>
	
#include <dirent.h>

#include <signal.h>

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

uint64_t RunServers::_clientBufferSize = 8192; // 8KB
uint64_t RunServers::_ramBufferLimit = 65536;
void RunServers::cleanupServer()
{
}

RunServers::~RunServers()
{
    close(_epfd);
}

void RunServers::createServers(vector<ConfigServer> &configs)
{
    for (ConfigServer &config : configs)
    {
        _servers.push_back(make_unique<Server>(Server(config)));
    }
    // Server::createListeners(_servers);
}

#include <chrono>
#include <thread>
#include <ctime> 
// void clocking()
// {
//     auto start = chrono::system_clock::now();
//     // Some computation here
//     auto end = chrono::system_clock::now();
 
//     chrono::duration<double> elapsed_seconds = end-start;
//     time_t end_time = chrono::system_clock::to_time_t(end);

//     Logger::log(INFO, "finished computation at ", ctime(&end_time)
//              , "elapsed time: ", elapsed_seconds.count(), "s"); //testlog

// }

void RunServers::addStdinToEpoll()
{
    int stdin_fd = 0; // stdin is always fd 0

    // Set stdin to non-blocking
    if (make_socket_non_blocking(stdin_fd) == false)
    {
        Logger::log(ERROR, "Failed to set stdin non-blocking");
        return;
    }

    // Add stdin to epoll
    struct epoll_event current_event;
    current_event.data.fd = stdin_fd;
    current_event.events = EPOLLIN;
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, stdin_fd, &current_event) == -1)
    {
        Logger::log(ERROR, "epoll_ctl (stdin): ", strerror(errno));
        return;
    }
}

int RunServers::runServers()
{
    epollInit(_servers); // need throw protection
    addStdinToEpoll();
    // clocking();
    while (g_signal_status == 0)
    {
        int eventCount;
        for (auto it = _clients.begin(); it != _clients.end();)
		{
			unique_ptr<Client> &client = it->second;
			++it;
			if (client->_disconnectTime <= chrono::steady_clock::now())
            {
                // cout << "disconnectTime: "
                //         << chrono::duration_cast<chrono::milliseconds>(client->_disconnectTime.time_since_epoch()).count()
                //         << " ms" << endl;
                // cout << "now: "
                //         << chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count()
                //         << " ms" << endl;
				cleanupClient(*client);
            }
		}
        // cout << "Blocking and waiting for epoll event..." << endl;
        eventCount = epoll_wait(_epfd, _events.data(), FD_LIMIT, DISCONNECT_DELAY_SECONDS);
        if (eventCount == -1) // only goes wrong with EINTR(signals)
        {
            if (errno == EINTR)
                break ;
            throw runtime_error(string("Server epoll_wait: ") + strerror(errno));
        }
        try
        {
            // cout << "event count "<<  eventCount << endl;
            handleEvents(static_cast<size_t>(eventCount));
        }
        catch(const exception& e)
        {
            Logger::log(ERROR, "Exception in handleEvents: ", e.what()); //testlog
        }
    }
    Logger::log(INFO, "Server shutting down gracefully...");
    return 0;
}

bool RunServers::runHandleTransfer(struct epoll_event &currentEvent)
{
    int eventFD = currentEvent.data.fd;
    for (auto it = _handle.begin(); it != _handle.end(); ++it)
    {
        if ((*it)->_client._fd == eventFD)
        {
            HandleTransfer &handle = **it;
            Client &client = handle._client;
            _clients[client._fd]->setDisconnectTime(DISCONNECT_DELAY_SECONDS);
            bool finished = false;
            if (currentEvent.events & EPOLLOUT)
                finished = handle.handleGetTransfer();
            else if (currentEvent.events & EPOLLIN)
            {
                if ((*it)->getIsChunk() == false)
                    finished = handle.handlePostTransfer(true);
                else
                {
                    handle.appendToBody();
                    finished = handle.handleChunkTransfer();
                }
            }
            if (finished == true)
            {
                if (client._keepAlive == false)
                {
                    // cleanupClient(*_clients[(*it)->_client._fd]);
                    if (_clients[(*it)->_client._fd]->_isCgi == false)
                        cleanupClient(*_clients[(*it)->_client._fd]);
                }
                else
                {
                    _handle.erase(it);
                    clientHttpCleanup(client);
                }
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
            {
                if ((*it)->readFromCgiTransfer() == true)
                {
                    Client &client = (*it)->_client;
                    if (client._keepAlive == false)
                    {
                        cleanupClient(client);
                    }
                    else
                    {
                        clientHttpCleanup(client);
                    }

                    _handleCgi.erase(it);
                }
            }
            return true;
        }
    }
    return false;
}

void RunServers::handleEvents(size_t eventCount)
{
    // int errHndl = 0;
    for (size_t i = 0; i < eventCount; ++i)
    {
        try
        {
            struct epoll_event &currentEvent = _events[i];
            int eventFD = currentEvent.data.fd;

            if (eventFD == 0 && (currentEvent.events & EPOLLIN)) {
                char buffer[1024];
                ssize_t bytesRead = read(0, buffer, sizeof(buffer) - 1);
                if (bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                }
                int snooze = stoi(buffer, nullptr, 16);
                if (snooze > 0 && snooze < 20)
                {
                    this_thread::sleep_for(chrono::seconds(snooze));
                }
                // processStdinInput(); // Your function to handle terminal input
                continue ;
            }
            if ((currentEvent.events & (EPOLLERR | EPOLLHUP)) ||
                !(currentEvent.events & (EPOLLIN | EPOLLOUT)))
            {
                if (currentEvent.events & EPOLLERR)
                    Logger::log(DEBUG, "EPOLLERR detected on fd ", static_cast<int>(currentEvent.data.fd), " (events: ", static_cast<int>(currentEvent.events), ")");
                if (currentEvent.events & EPOLLHUP)
                    Logger::log(DEBUG, "EPOLLHUP detected on fd ", static_cast<int>(currentEvent.data.fd), " (events: ", static_cast<int>(currentEvent.events), ")");
                if (!(currentEvent.events & (EPOLLIN | EPOLLOUT)))
                    Logger::log(DEBUG, "Unexpected epoll event on fd ", static_cast<int>(currentEvent.data.fd), " (events: ", static_cast<int>(currentEvent.events), ") - no EPOLLIN or EPOLLOUT");
                auto clientIt = _clients.find(currentEvent.data.fd);
                if (clientIt != _clients.end() && clientIt->second)
                {
                    cleanupClient(*clientIt->second);
                }
                else
                {
                    // Just cleanup the FD if no client exists
                    cleanupFD(currentEvent.data.fd);
                }
                continue;
            }
            if (find(_listenFDS.begin(), _listenFDS.end(), eventFD) != _listenFDS.end())
            {
                acceptConnection(eventFD);
            }
            // cout << '4' << endl;
            if (runHandleTransfer(currentEvent) == true || \
                runCgiHandleTransfer(currentEvent) == true)
                continue;
            // cout << '5' << endl;

            if ((_clients.find(eventFD) != _clients.end()) &&
                (currentEvent.events == EPOLLIN))
            {
                // cout << "5in" << endl;
                processClientRequest(*_clients[eventFD].get());
                continue;
            }
            // cout << '6' << endl;
        }
        catch (const ErrorCodeClientException &e)
        {
            e.handleErrorClient();  //TODO anything throwing in here stops the server
        }
    }
}

void RunServers::insertHandleTransfer(unique_ptr<HandleTransfer> handle)
{
    _handle.push_back(move(handle));
}

void RunServers::insertHandleTransferCgi(unique_ptr<HandleTransfer> handle)
{
    _handleCgi.push_back(move(handle));
}


// bool RunServers::handlingSend(HandleTransfer &ht)
// {

// }

void RunServers::getExecutableDirectory()
{
    try
    {
        _serverRootDir = filesystem::canonical("/proc/self/exe").parent_path().string();
    }
    catch (const exception& e)
    {
        Logger::logExit(ERROR, "Cannot determine executable directory: " + string(e.what()));
    }
}
