#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
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

#include <chrono>
#include <ctime> 
void clocking()
{
    auto start = chrono::system_clock::now();
    // Some computation here
    auto end = chrono::system_clock::now();
 
    chrono::duration<double> elapsed_seconds = end-start;
    time_t end_time = chrono::system_clock::to_time_t(end);
 
    cout << "finished computation at " << ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s"
              << endl;
}

int RunServers::runServers()
{
    epollInit();
    clocking();
    while (g_signal_status == 0)
    {
        int eventCount;
        
        std::cout << "Blocking and waiting for epoll event..." << std::endl;
        FileDescriptor::keepAliveCheck();
        eventCount = epoll_wait(_epfd, _events.data(), FD_LIMIT, 1000);
        if (eventCount == -1) // only goes wrong with EINTR(signals)
        {
            break ;
            throw runtime_error(string("Server epoll_wait: ") + strerror(errno));
        }
        try
        {
            handleEvents(static_cast<size_t>(eventCount));
        }
        catch(const ErrorCodeClientException& e)
        {
            e.handleErrorClient();
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
            if (find(listeners.begin(), listeners.end(), eventFD) != listeners.end() && currentEvent.events == EPOLLIN)
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
        // if (currentEvent.events & EPOLLOUT)
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
            ht._fileBuffer.append(buff, _bytesRead);
            
            if (ht._epollout_enabled == false)
            {
                setEpollEvents(ht._clientFD, EPOLL_CTL_MOD, EPOLLOUT);
                ht._epollout_enabled = true;
            }
        }
        if (_bytesRead == 0 || ht._bytesReadTotal >= ht._fileSize)
        {
            // EOF reached, close file descriptor if needed
            FileDescriptor::closeFD(ht._fd);
            ht._fd = -1;
        }
    }
    ssize_t sent = send(ht._clientFD, ht._fileBuffer.c_str() + ht._offset, ht._fileBuffer.size() - ht._offset, 0);
    if (sent == -1)
        throw ClientException(string("handlingTransfer send: ") + strerror(errno));
    size_t _sent = static_cast<size_t>(sent);
    ht._offset += _sent;
    if (ht._offset >= ht._fileSize + ht._headerSize) // TODO only between boundary is the filesize
    {
        // string boundary = "--" + ht._boundary + "--\r\n\r\n";
        if (FileDescriptor::containsClient(ht._clientFD) == false)
        {
            RunServers::cleanupFD(ht._clientFD);
            return true;
        }
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