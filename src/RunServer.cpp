#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>
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
// unordered_map<int, string> RunServers::_fdBuffers;
ServerList RunServers::_servers;
vector<unique_ptr<HandleTransfer>> RunServers::_handle;
// vector<int> RunServers::_connectedClients;
unordered_map<int, unique_ptr<Client>> RunServers::_clients;

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
        
        // std::cout << "Blocking and waiting for epoll event..." << std::endl;
        // for (auto it = _clients.begin(); it != _clients.end();)
		// {
		// 	unique_ptr<Client> &client = it->second;
		// 	++it;
		// 	if (client->_keepAlive == false || client->_disconnectTime <= chrono::steady_clock::now())
		// 		cleanupClient(*client);
		// }
		
        eventCount = epoll_wait(_epfd, _events.data(), FD_LIMIT, 2500);
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

bool RunServers::runHandleTransfer(struct epoll_event &currentEvent)
{
    int eventFD = currentEvent.data.fd;
    for (auto it = _handle.begin(); it != _handle.end(); ++it)
    {
        if ((*it)->_client._fd == eventFD)
        {
            HandleTransfer &handle = **it;
            _clients[(*it)->_client._fd]->setDisconnectTime(disconnectDelaySeconds);
            bool finished = false;
            if (currentEvent.events & EPOLLOUT)
                finished = handle.handleGetTransfer();
            else if (currentEvent.events & EPOLLIN)
                finished = handle.handlePostTransfer();
            if (finished == true)
            {
                if (_clients[(*it)->_client._fd]->_keepAlive == false)
                    cleanupClient(*_clients[(*it)->_client._fd]);
                it = _handle.erase(it);
                setEpollEvents((*it)->_client._fd, EPOLL_CTL_MOD, EPOLLIN);
                // std::cout << "current epoll event:" << currentEvent.events << std::endl;
            }
            return true;
        }
    }
    return false;
}

void RunServers::handleEvents(size_t eventCount)
{
    // int errHndl = 0;
    // std::cout << "\t" << eventCount << std::endl;
    for (size_t i = 0; i < eventCount; ++i)
    {
		struct epoll_event &currentEvent = _events[i];
        int eventFD = currentEvent.data.fd;
        if (currentEvent.events & EPOLLHUP) // turned off epollerr
        {
            cout << "client closed connection on fd: " << eventFD << std::endl;
            cleanupClient(*_clients[eventFD].get());
            continue ;
        }
        if (currentEvent.events & EPOLLERR || !(currentEvent.events & (EPOLLIN | EPOLLOUT)))
        {
            int socket_error = 0;
            socklen_t len = sizeof(socket_error);
            if (getsockopt(currentEvent.data.fd, SOL_SOCKET, SO_ERROR, &socket_error, &len) == 0)
            {
                std::cerr << "Socket error: " << strerror(socket_error) << std::endl;
            }
            std::cerr << "epoll error on fd " << currentEvent.data.fd
                      << " (events: " << currentEvent.events << ")" << std::endl;
            cleanupFD(currentEvent.data.fd);
            exit(1);
            continue;
        }
        // bool handltransfers = true;
        for (const unique_ptr<Server> &server : _servers)
        {
            vector<int> &listeners = server->getListeners();
            if (find(listeners.begin(), listeners.end(), eventFD) != listeners.end() && currentEvent.events == EPOLLIN)
            {
                // handltransfers = false;
                acceptConnection(server);
                continue ;
            }
        }
        if (runHandleTransfer(currentEvent) == true)
            continue ;
		if ((_clients.find(eventFD) != _clients.end()) &&
			(currentEvent.events & EPOLLIN))
		{
			processClientRequest(*_clients[eventFD].get());
			continue ;
		}
        
    }
}

void RunServers::insertHandleTransfer(unique_ptr<HandleTransfer> handle)
{
    _handle.push_back(move(handle));
    // TODO removal functions
}




// bool RunServers::handlingSend(HandleTransfer &ht)
// {

// }