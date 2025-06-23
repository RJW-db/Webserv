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

void RunServers::handleEvents(size_t eventCount)
{
    // int errHndl = 0;
    // std::cout << "\t" << eventCount << std::endl;
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
		for (auto it = _handle.begin(); it != _handle.end(); ++it)
		{
			if ((*it)->_client._fd == eventFD)
			{
				_clients[(*it)->_client._fd]->setDisconnectTime(disconnectDelaySeconds);
				bool finished = false;
				if (currentEvent.events & EPOLLOUT)
					finished = handleGetTransfer(**it);
				else if (currentEvent.events & EPOLLIN)
					finished = handlePostTransfer(**it);
				if (finished == true)
				{
					if (_clients[(*it)->_client._fd]->_keepAlive == false)
						cleanupClient(*_clients[(*it)->_client._fd]);
					_handle.erase(it);
				}
				return;
			}
		}
		if ((_clients.find(eventFD) != _clients.end()) &&
			(currentEvent.events & EPOLLIN))
		{
			processClientRequest(*_clients[eventFD].get());
			return;
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

        }
    }
}

void RunServers::insertHandleTransfer(unique_ptr<HandleTransfer> handle)
{
    _handle.push_back(move(handle));
    // TODO removal functions
}

bool RunServers::handleGetTransfer(HandleTransfer &ht)
{
    char buff[CLIENT_BUFFER_SIZE];
    if (ht._fd != -1)
    {
        ssize_t bytesRead = read(ht._fd, buff, CLIENT_BUFFER_SIZE);
        if (bytesRead == -1)
            throw ClientException(string("handlingTransfer read: ") + strerror(errno));
        size_t _bytesRead = static_cast<size_t>(bytesRead);
        ht._bytesReadTotal += _bytesRead;
        if (_bytesRead > 0)
        {
            ht._fileBuffer.append(buff, _bytesRead);
            
            if (ht._epollout_enabled == false)
            {
                setEpollEvents(ht._client._fd, EPOLL_CTL_MOD, EPOLLOUT);
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
    ssize_t sent = send(ht._client._fd, ht._fileBuffer.c_str() + ht._offset, ht._fileBuffer.size() - ht._offset, 0);
    if (sent == -1)
        throw ClientException(string("handlingTransfer send: ") + strerror(errno));
    size_t _sent = static_cast<size_t>(sent);
    ht._offset += _sent;
    if (ht._offset >= ht._fileSize + ht._headerSize) // TODO only between boundary is the filesize
    {
        setEpollEvents(ht._client._fd, EPOLL_CTL_MOD, EPOLLIN);
        ht._epollout_enabled = false;
        return true;
    }
    return false;
}

bool RunServers::handlePostTransfer(HandleTransfer &ht)
{
	char buff[CLIENT_BUFFER_SIZE];
	ssize_t bytesReceived = recv(ht._client._fd, buff, sizeof(buff), 0);
	size_t byteswrite = static_cast<size_t>(bytesReceived);
	ssize_t bytesWritten = 0;
	if (bytesReceived == -1)
	{
		cleanupClient(ht._client);
		return true;
	}
	else if (bytesReceived != 0)
	{
		if (bytesReceived > ht._fileSize - ht._bytesWrittenTotal)
			byteswrite = ht._fileSize - ht._bytesWrittenTotal;
		bytesWritten = write(ht._fd, buff, byteswrite);
		if (bytesWritten == -1)
		{
			// remove and filedesciptor
			// if (!handle._filename.empty()) // assuming HandleTransfer has a _filename member
			// 	remove(handle._filename.data());
			FileDescriptor::closeFD(ht._fd);
			throw ErrorCodeClientException(ht._client, 500, string("write failed HandlePostTransfer: ") + strerror(errno), ht._client._location.getErrorCodesWithPage());
			
		}
		ht._bytesWrittenTotal += bytesWritten;
	}
	if (ht._bytesWrittenTotal == ht._fileSize)
	{
		ht._fileBuffer.append(buff + bytesWritten, bytesReceived - bytesWritten);
		if (ht._fileBuffer.find("--" + string(ht._client._bodyBoundary) + "--\r\n") == 2)
		{
			FileDescriptor::closeFD(ht._fd);
			string ok = HttpRequest::HttpResponse(200, "", 0);
			send(ht._client._fd, ok.data(), ok.size(), 0);
			return true;
		}
	}
	return false;
}


// bool RunServers::handlingSend(HandleTransfer &ht)
// {

// }