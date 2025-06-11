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
    try
    {
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
            // fprintf(stdout, "Received epoll event\n");
            handleEvents(static_cast<size_t>(eventCount));
    
        }
        cout << "\rGracefully stopping... (press Ctrl+C again to force)" << endl;
    }
    catch(const exception& e) // catch-all, will determine whether CleanupClient needs to be called or not
    {
        cerr << e.what() << endl;
    }

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
        if ((currentEvent.events & (EPOLLERR | EPOLLHUP))||
        !(currentEvent.events & EPOLLIN))
        {
            std::cerr << "epoll error on fd " << currentEvent.data.fd
            << " (events: " << currentEvent.events << ")" << std::endl;
            cleanupFD(currentEvent.data.fd);
            continue;
        }

        bool handltransfers = true;
        int clientFD = currentEvent.data.fd;
        for (const unique_ptr<Server> &server : _servers)
        {
            vector<int> &listeners = server->getListeners();
            if (find(listeners.begin(), listeners.end(), clientFD) != listeners.end())
            {
                handltransfers = false;
                acceptConnection(server);
            }
        }
        if (find(_connectedClients.begin(), _connectedClients.end(), clientFD) != _connectedClients.end())
        {
            processClientRequest(clientFD);
            handltransfers = false;
        }

        if (handltransfers == true)
        {
            
        }
        // if (finishes_send_read() == false)
        // if (found == false)
        // 	processClientRequest(server, clientFD);
    }
}

void RunServers::insertHandleTransfer(unique_ptr<HandleTransfer> handle)
{
    _handle.push_back(move(handle));
    // TODO removal functions
}

void RunServers::handlingTransfer(HandleTransfer &ht)
{
    ssize_t sent = send(ht._clientFD, ht._fileBuffer.c_str() + ht._offset, ht._fileBuffer.length() - ht._offset, 0);
    if (sent == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // Can't send now, wait for EPOLLOUT
            setEpollEvents(ht._clientFD, EPOLLIN | EPOLLOUT);
            ht._epollout_enabled = true;
        }
        else
        {
            // Handle error
        }
        return;
    }

    size_t _sent = static_cast<size_t>(sent);
    if (_sent < ht._fileBuffer.length() - ht._offset)
    {
        // Partial send, update offset and wait for EPOLLOUT
        ht._offset += _sent;
        if (ht._offset < ht._fileBuffer.length())
        {
            // Still data left to send
            if (ht._epollout_enabled == false)
            {
                setEpollEvents(ht._clientFD, EPOLLIN | EPOLLOUT);
                ht._epollout_enabled = true;
            }
        }
        else
        {
            // All data sent, disable EPOLLOUT
            setEpollEvents(ht._clientFD, EPOLLIN);
            ht._epollout_enabled = false;
        }
    }
    else
    {
        // All data sent, no need for EPOLLOUT
        setEpollEvents(ht._clientFD, EPOLLIN);
    }
}
