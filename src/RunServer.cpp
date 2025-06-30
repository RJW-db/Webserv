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
// FileDescriptor RunServers::_fds;
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
        
        std::cout << "Blocking and waiting for epoll event..." << std::endl;
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
            // std::cout << "event count "<<  eventCount << std::endl;
            handleEvents(static_cast<size_t>(eventCount));
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
            Client &client = handle._client;
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

void RunServers::handleEvents(size_t eventCount)
{
    // int errHndl = 0;
    // std::cout << "\t" << eventCount << std::endl;
    for (size_t i = 0; i < eventCount; ++i)
    {
        try
        {
            struct epoll_event &currentEvent = _events[i];
            int eventFD = currentEvent.data.fd;

            if ((currentEvent.events & (EPOLLERR | EPOLLHUP)) ||
                !(currentEvent.events & (EPOLLIN | EPOLLOUT)))
            {
                std::cerr << "epoll fault on fd " << currentEvent.data.fd
                          << " (events: " << currentEvent.events << ")" << std::endl;
                // std::cout << errno << std::endl;
                cleanupClient(*_clients[currentEvent.data.fd].get());
                continue;
            }
            for (const unique_ptr<Server> &server : _servers)
            {
                vector<int> &listeners = server->getListeners();
                auto it = find(listeners.begin(), listeners.end(), eventFD);
                if (it != listeners.end() && currentEvent.events == EPOLLIN)
                {
                    // handltransfers = false;
                    acceptConnection(*it);
                    break;
                }
            }
            // std::cout << '4' << std::endl;

            if (runHandleTransfer(currentEvent) == true)
                continue;
            // std::cout << '5' << std::endl;

            if ((_clients.find(eventFD) != _clients.end()) &&
                (currentEvent.events & EPOLLIN))
            {
                // std::cout << "5in" << std::endl;
                processClientRequest(*_clients[eventFD].get());
                continue;
            }
            // std::cout << '6' << std::endl;
        }
        catch (const ErrorCodeClientException &e)
        {
            e.handleErrorClient();
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