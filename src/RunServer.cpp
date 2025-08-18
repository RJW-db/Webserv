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
#include <sys/wait.h>

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
void clocking();
void RunServers::setupEpoll()
{
    epollInit(_servers);
    // if (TERMINAL_DEBUG)
    //     addStdinToEpoll();
}


void RunServers::addStdinToEpoll()
{
    int stdin_fd = 0;

    if (makeSocketNonBlocking(stdin_fd) == false)
        Logger::logExit(ERROR, "Failed to set stdin to non-blocking mode");

    struct epoll_event current_event;
    current_event.data.fd = stdin_fd;
    current_event.events = EPOLLIN;
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, stdin_fd, &current_event) == -1)
        Logger::logExit(ERROR, "epoll_ctl (stdin): ", strerror(errno));
}

void RunServers::closeHandles(pid_t pid)
{
    for (auto it = _handleCgi.begin(); it != _handleCgi.end();)
    {
        if ((*it)->_client._pid == pid)
        {
            cleanupFD((*it)->_fd);
            it =_handleCgi.erase(it);
            continue ;
        }
        ++it;
    }
}

// void RunServers::checkCgiDisconnect()
// {
//     for (std::pair<const int, std::unique_ptr<Client>> &clientPair : _clients)
//     {
//         unique_ptr<Client> &client = clientPair.second;
//         if (client->_cgiPid != -1)
//         {
//             int exit_code;
//             pid_t result = waitpid(client->_cgiPid, &exit_code, WNOHANG);
//             if (result > 0)
//             {
//                 if (WIFEXITED(exit_code))
//                 {
//                     std::cout << "CGI process " << client->_cgiPid << " exited with status " << WEXITSTATUS(exit_code) << " on client with fd " << client->_fd << std::endl;
//                     closeHandles(client->_cgiPid);
//                     // if (WEXITSTATUS(exit_code) > 0)
//                         // throw ErrorCodeClientException(*client, 500, "Cgi error");
//                     if (client->_keepAlive == false)
//                         cleanupClient(*client);
//                     else
//                         clientHttpCleanup(*client);
//                 }
//             }
//         }
//     }
// }


void RunServers::checkCgiDisconnect()
{
    for (auto it = _handleCgi.begin(); it != _handleCgi.end();)
    {
        int exit_code;
        Client &client = (*it)->_client;

        pid_t result = waitpid(client._pid, &exit_code, WNOHANG);
        if (result == 0)
        {
            ++it;
        }
        else
        {
            if (WIFEXITED(exit_code))
            {
                Logger::log(INFO, "cgi process with pid: ", client._pid, " exited with status: ", WEXITSTATUS(exit_code)); //testlog
                HandleTransfer& currentTransfer = *(*it);
                if (currentTransfer._handleType == HANDLE_WRITE_TO_CGI_TRANSFER)
                {
                    cleanupFD(currentTransfer._fd);
                    it = _handleCgi.erase(it);
                    continue ;
                }
                if (WEXITSTATUS(exit_code) > 0)
                    throw ErrorCodeClientException(client, 500, "Cgi error");
                bool finished = currentTransfer.readFromCgiTransfer();
                if (finished)
                {
                    it = _handleCgi.erase(it);
                    if (client._keepAlive == false)
                        cleanupClient(client);
                    else
                        clientHttpCleanup(client);
                }
                else
                    ++it;
            }
            else
            {
                ++it;
            }
        }
    }
}

void RunServers::checkClientDisconnects()
{
    try
    {
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
    }
    catch (const std::exception &e)
    {
        Logger::log(ERROR, "Exception in checkClientDisconnects: ", e.what()); // testlog
    }
}

int RunServers::runServers()
{
    while (g_signal_status == 0)
    {
        int eventCount;

        // cout << "Blocking and waiting for epoll event..." << endl;
        checkClientDisconnects();
        checkCgiDisconnect();
        eventCount = epoll_wait(_epfd, _events.data(), FD_LIMIT, DISCONNECT_DELAY_SECONDS);
        if (eventCount == -1) // only goes wrong with EINTR(signals)
        {
            if (errno == EINTR)
                break ;
            Logger::logExit(ERROR, "Server epoll_wait: ", strerror(errno));
        }
        try
        {
            // cout << "event count "<<  eventCount << endl;
            handleEvents(static_cast<size_t>(eventCount));
        }
        catch(Logger::ErrorLogExit&)
        {
            Logger::logExit(ERROR, "Restart now or finish existing clients and exit");
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
                if (client._keepAlive == false && client._isCgi == false)
                {
                    std::cout << "cleaning here" << std::endl; //testcout
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
                    Logger::log(WARN, "EPOLLERR detected on fd ", static_cast<int>(eventFD), " (events: ", static_cast<int>(currentEvent.events), ")");
                if (currentEvent.events & EPOLLHUP)
                    Logger::log(WARN, "EPOLLHUP detected on fd ", static_cast<int>(eventFD), " (events: ", static_cast<int>(currentEvent.events), ")");
                if (!(currentEvent.events & (EPOLLIN | EPOLLOUT)))
                    Logger::log(WARN, "Unexpected epoll event on fd ", static_cast<int>(eventFD), " (events: ", static_cast<int>(currentEvent.events), ") - no EPOLLIN or EPOLLOUT");
                auto clientIt = _clients.find(eventFD);
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
        char buf[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len == -1)
            Logger::logExit(ERROR, "readlink failed: ", strerror(errno));
        buf[len] = '\0';

        std::filesystem::path exePath(buf);
        _serverRootDir = exePath.parent_path().string();
    }
    catch (const exception& e)
    {
        Logger::logExit(ERROR, "Cannot determine executable directory: " + string(e.what()));
    }
}
