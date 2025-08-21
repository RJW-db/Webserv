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
        Logger::logExit(ERROR, "Server error" , '-', "Failed to set stdin to non-blocking mode");

    struct epoll_event current_event;
    current_event.data.fd = stdin_fd;
    current_event.events = EPOLLIN;
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, stdin_fd, &current_event) == -1)
        Logger::logExit(ERROR, "Server error" , '-', "epoll_ctl (stdin): ", strerror(errno));
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

void RunServers::checkCgiDisconnect()
{
    for (auto it = _handleCgi.begin(); it != _handleCgi.end();)
    {
        int exit_code;
        Client &client = (*it)->_client;

        if (client._cgiClosing == true)
        {
            Logger::log(DEBUG, +(*it)->_handleType, ", Child process for client ", client._fd, " has closed its pipes"); //testlog
            cleanupFD((*it)->_fd);
            it = _handleCgi.erase(it);
            continue ;
        }
        // std::cout << "_handleType " << +((*it)->_handleType) << std::endl; //testcout
        pid_t result = waitpid(client._pid, &exit_code, WNOHANG);
        // Logger::log(DEBUG, "result: ", result); //testlog
        if (result == 0)
        {
            if (client._disconnectTimeCgi <= chrono::steady_clock::now())
            {
                client._cgiClosing = true;
                Logger::log(DEBUG, "client._pid: ", client._pid); //testlog
                // if (client._cgiClosing == true)
                kill(client._pid, SIGTERM);
                Logger::log(DEBUG, "Killed child"); //testlog
                Logger::log(DEBUG, "client._pid: ", client._pid); //testlog
                cleanupFD((*it)->_fd);
                it = _handleCgi.erase(it);
                throw ErrorCodeClientException(client, 500, "Reading from CGI failed");
                continue;
            }
            ++it;
        }
        else
        {
            std::cout << "result: " << result << std::endl; //testcout
            if (WIFEXITED(exit_code))
            {
                Logger::log(INFO, "cgi process with pid: ", client._pid, " exited with status: ", WEXITSTATUS(exit_code)); //testlog
                HandleTransfer& currentTransfer = *(*it);
                // Logger::log(DEBUG, "handletype: ", currentTransfer._handleType); //testlog
                if (currentTransfer._handleType == HANDLE_WRITE_TO_CGI_TRANSFER ||
                    currentTransfer._handleType == HANDLE_READ_FROM_CGI_TRANSFER)
                {
                    cleanupFD(currentTransfer._fd);
                    client._cgiClosing = true;
                    Logger::log(DEBUG, +(*it)->_handleType, ", Child process for client ", client._fd, " has closed its pipes"); //testlog
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
                client->_cgiClosing = true;
                checkCgiDisconnect();
                cleanupClient(*client);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "not here" << std::endl; //testcout
        Logger::log(ERROR, "Server error", '-', "Exception in checkClientDisconnects: ", e.what());
    }
}

void RunServers::removeHandlesWithFD(int fd)
{
    for(auto it = _handleCgi.begin(); it != _handleCgi.end(); ++it)
    {
        if ((*it)->_fd == fd)
        {
            _handleCgi.erase(it);
            return;
        }
    }
}

void RunServers::runServers()
{
    while (g_signal_status == 0)
    {
        int eventCount;

        // cout << "Blocking and waiting for epoll event..." << endl;
        try
        {
            checkCgiDisconnect();
            checkClientDisconnects();
        }
        catch (const ErrorCodeClientException &e)
        {
            e.handleErrorClient();  //TODO anything throwing in here stops the server
        }
        catch(Logger::ErrorLogExit&)
        {
            Logger::logExit(ERROR, "Server error", '-', "Restart now or finish existing clients and exit");
        }
        catch(const exception& e)
        {
            Logger::log(ERROR, "Server error", '-', "Exception in handleEvents: ", e.what());
        }
        

        eventCount = epoll_wait(_epfd, _events.data(), FD_LIMIT, DISCONNECT_DELAY_SECONDS);
        if (eventCount == -1) // only goes wrong with EINTR(signals)
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
            {
                if (handle._handleType == HANDLE_GET_TRANSFER)
                    finished = handle.handleGetTransfer();
                else
                    finished = handle.sendToClientTransfer();
            }
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
                    // if (client._keepAlive == false)
                    // {
                    //     cleanupClient(client);
                    // }
                    // else
                    // {
                        clientHttpCleanup(client);
                    // }
                    // _handleCgi.erase(it);
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
                int snooze = stoi(buffer, nullptr, 16); //TODO not protectect sam
                if (snooze > 0 && snooze < 20)
                {
                    this_thread::sleep_for(chrono::seconds(snooze));
                }
                // processStdinInput(); // Your function to handle terminal input
                continue ;
            }
            // Logger::log(INFO/*should be INFO*/, "Epoll event", static_cast<int>(eventFD), ":clientFD", "EPOLLERR (events: ", static_cast<int>(currentEvent.events), ')');
            // Logger::log(WARN, "Epoll event", "     " + to_string(eventFD) + ":clientFD", "Unexpected (events: ", static_cast<int>(currentEvent.events), ") - no EPOLLIN or EPOLLOUT");
            if ((currentEvent.events & (EPOLLERR | EPOLLHUP)) ||
                !(currentEvent.events & (EPOLLIN | EPOLLOUT)))
            {
                // Indicates a socket error (e.g. connection reset, write to closed socket, etc.)
                if (currentEvent.events & EPOLLERR)
                    Logger::log(INFO/*should be INFO*/, "Epoll event", static_cast<int>(eventFD), ":clientFD", "EPOLLERR (events: ", static_cast<int>(currentEvent.events), ')');
                // Indicates the socket was closed (the remote side hung up)
                if (currentEvent.events & EPOLLHUP)
                    Logger::log(INFO/*should be INFO*/, "Epoll event", static_cast<int>(eventFD), ":clientFD", "EPOLLHUP (events: ", static_cast<int>(currentEvent.events), ')');
                // Indicates the socket is not ready for reading or writing (no EPOLLIN or EPOLLOUT set)
                if (!(currentEvent.events & (EPOLLIN | EPOLLOUT)))
                    Logger::log(WARN, "Epoll event", to_string(eventFD) + ":clientFD", "Unexpected (events: ", static_cast<int>(currentEvent.events), ") - no EPOLLIN or EPOLLOUT");
                    // Logger::log(WARN, "Unexpected epoll event on                 eventFD: ", static_cast<int>(eventFD), "  (events: ", static_cast<int>(currentEvent.events), ") - no EPOLLIN or EPOLLOUT");
                auto clientIt = _clients.find(eventFD);
                if (clientIt != _clients.end() && clientIt->second)
                {
                    cleanupClient(*clientIt->second);
                }
                else
                {
                    removeHandlesWithFD(eventFD);
                    // Just cleanup the FD if no client exists
                    cleanupFD(eventFD);
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
            Logger::logExit(ERROR, "Server error", '-', "readlink failed: ", strerror(errno));
        buf[len] = '\0';

        std::filesystem::path exePath(buf);
        _serverRootDir = exePath.parent_path().string();
    }
    catch (const exception& e)
    {
        Logger::logExit(ERROR, "Server error", '-', "Cannot determine executable directory: " + string(e.what()));
    }
}
