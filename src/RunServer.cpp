#include <RunServer.hpp>
#include <iostream>
#include <FileDescriptor.hpp>

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

#include <ctime>
	
#include <dirent.h>

#include <signal.h>
extern volatile sig_atomic_t g_signal_status;

// Static member variables
FileDescriptor RunServers::_fds;
int RunServers::_epfd = -1;
array<struct epoll_event, FD_LIMIT> RunServers::_events;
unordered_map<int, string> RunServers::_fdBuffers;
unordered_map<int, ClientRequestState> RunServers::_clientStates;
ServerList RunServers::_servers;
vector<unique_ptr<HandleTransfer>> RunServers::_handle;
vector<int> RunServers::_connectedClients;

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
    Server::createListeners(_servers);
    
}

int RunServers::runServers()
{
    epollInit();
    std::srand(std::time(NULL));
    while (g_signal_status == 0)
    {
        int eventCount;

        fprintf(stdout, "Blocking and waiting for epoll event...\n");
        eventCount = epoll_wait(_epfd, _events.data(), FD_LIMIT, -1);
        if (eventCount == -1) // for use only goes wrong with EINTR(signals)
        {
            break ;
            throw runtime_error(string("Server epoll_wait: ") + strerror(errno));
        }
        try
        {
            handleEvents(static_cast<size_t>(eventCount));
        }
        catch(const ClientException &e)
        {
            std::cerr << e.what() << '\n';
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    cout << "\rGracefully stopping... (press Ctrl+C again to force)" << endl;
    return 0;
}

void RunServers::handleEvents(size_t eventCount)
{
    // int errHndl = 0;
    for (size_t i = 0; i < eventCount; ++i)
    {
        struct epoll_event &currentEvent = _events[i];
        // if ((currentEvent.events & EPOLLERR) ||
        //     (currentEvent.events & EPOLLHUP) ||
        //     (currentEvent.events & EPOLLIN) == 0)
        // {
        if ((currentEvent.events & (EPOLLERR | EPOLLHUP)) ||
           !(currentEvent.events & (EPOLLIN | EPOLLOUT)))

        {
            std::cerr << "epoll error on fd " << currentEvent.data.fd
            << " (events: " << currentEvent.events << ")" << std::endl;
            cleanupFD(currentEvent.data.fd);
            continue;
        }

        // bool handltransfers = true;
        int eventFD = currentEvent.data.fd;
        for (const unique_ptr<Server> &server : _servers)
        {
            vector<int> &listeners = server->getListeners();
            if (find(listeners.begin(), listeners.end(), eventFD) != listeners.end())
            {
                // handltransfers = false;
                acceptConnection(server);
                return ;
            }
        }
        if ((find(_connectedClients.begin(), _connectedClients.end(), eventFD) != _connectedClients.end()) &&
            currentEvent.events & EPOLLIN)
        {
            processClientRequest(eventFD);
            return ;
            // handltransfers = false;
        }
        if (currentEvent.events & EPOLLOUT)
        {
            for (auto it = _handle.begin(); it != _handle.end(); ++it)
            {
                if ((*it)->_clientFD == eventFD)
                {
                    if (handlingTransfer(**it) == true)
                        _handle.erase(it);
                    return;
                }
            }
        }
        // else if (currentEvent.events & EPOLLOUT)
        // {
        //     for (auto it = _handle.begin(); it != _handle.end(); ++it)
        //     {
        //         if ((*it)->_clientFD == eventFD)
        //         {
        //             if (handlingSend(**it) == true)
        //                 _handle.erase(it);
        //             return;
        //         }
        //     }
        // }
    }
}

void RunServers::insertHandleTransfer(unique_ptr<HandleTransfer> handle)
{
    _handle.push_back(move(handle));
    // TODO removal functions
}

#define CHUNK_SIZE 8192 // 8KB
bool RunServers::handlingTransfer(HandleTransfer &ht)
{
    char buff[CHUNK_SIZE];
    if (ht._fd != -1)
    {
        ssize_t bytesRead = read(ht._fd, buff, CHUNK_SIZE);
        if (bytesRead == -1)
            throw ClientException(string("handlingTransfer read: ") + strerror(errno));
        size_t _bytesRead = static_cast<size_t>(bytesRead);
        ht._bytesReadTotal += _bytesRead;
        if (_bytesRead > 0)
        {
            ht._fileBuffer.append(buff);
            
            if (ht._epollout_enabled == false)
            {
                setEpollEvents(ht._clientFD, EPOLL_CTL_MOD, EPOLLIN | EPOLLOUT);
            }
        }
        if (_bytesRead == 0 || ht._bytesReadTotal >= ht._fileSize)
        {
            // EOF reached, close file descriptor if needed
            close(ht._fd);
            ht._fd = -1;
        }
    }
    std::cout << "buff:" << ht._fileBuffer << std::endl;
    ssize_t sent = send(ht._clientFD, ht._fileBuffer.c_str() + ht._offset, ht._fileBuffer.size() - ht._offset, 0);
    if (sent == -1)
        throw ClientException(string("handlingTransfer send: ") + strerror(errno));
    size_t _sent = static_cast<size_t>(sent);
    ht._offset += _sent;
    // if (_sent < ht._fileBuffer.size() - ht._offset)
    // {
    //     // Partial send, update offset and wait for EPOLLOUT
    //     // if (ht._offset >= ht._fileBuffer.size())
    //     // {
    //     //     // All data sent, disable EPOLLOUT
    //     //     setEpollEvents(ht._clientFD, EPOLL_CTL_MOD, EPOLLIN);
    //     //     ht._epollout_enabled = false;
    //     // }
    // }
    if (ht._offset >= ht._fileSize) // TODO only between boundary is the filesize
    {
        // string boundary = "--" + ht._boundary + "--\r\n\r\n";
        // send(ht._clientFD, boundary.c_str(), boundary.size(), 0);
        setEpollEvents(ht._clientFD, EPOLL_CTL_MOD, EPOLLIN);
        ht._epollout_enabled = false;
        return true;
    }
    return false;
}

// bool RunServers::handlingSend(HandleTransfer &ht)
// {

// }