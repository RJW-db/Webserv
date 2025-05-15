#include <Webserv.hpp>
#include <Parsing.hpp>
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
#include <sys/epoll.h>
#endif

#include <iostream>
#include <memory>
#include <vector>


// int              create_listener_socket(void);
// struct addrinfo *get_server_addrinfo(void);
// int              bind_to_socket(struct addrinfo *server);


int main()
{
    // poll_usages();
    // epoll_usage();
    // getaddrinfo_usage();
    // server();


	// Parsing sam("./config/default.conf");

	// exit(0);
	FileDescriptor fds;


    // ServerList servers;
	// std::vector<std::string> ports = {"8080", "8090"};
    // size_t amount_servers = 1;
	// for (size_t i = 0; i < amount_servers; ++i) {
	// 	servers.push_back(std::make_unique<Server>());
	// }

	tmp_t serverConfig[2];
	serverConfig[0] = (tmp_t){"Alpha", "8080"};
	// serverConfig[1] = (tmp_t){"Beta", "6789"};

    ServerList servers;
    size_t amount_servers = 1;
	for (size_t i = 0; i < amount_servers; ++i) {
		servers.push_back(std::make_unique<Server>(serverConfig + i));
	}

	Server::epollInit(servers);
    Server::runServers(servers, fds);
    return 0;
}

// void Server::acceptConnection(const std::unique_ptr<Server>& server)
// {

// }
void Server::handleEvents(ServerList& servers, FileDescriptor& fds, int eventCount)
{
    int errHndl = 0;
	for (int i = 0; i < eventCount; ++i)
	{
		struct epoll_event &currentEvent = _events[i];
		if ((currentEvent.events & EPOLLERR) ||
			(currentEvent.events & EPOLLHUP) ||
			(currentEvent.events & EPOLLIN) == 0)
		{
			fprintf(stderr, "epoll error\n");
			close(currentEvent.data.fd);
			continue;
		}

		for (const std::unique_ptr<Server>& server : servers)
		{
			if (server->_listener == currentEvent.data.fd)
			{
				while(1)
				{
					struct sockaddr in_addr;
					socklen_t in_len;
					int infd;
					char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

					in_len = sizeof(in_addr);
					infd = accept(server->_listener, &in_addr, &in_len);
					if(infd == -1)
					{
						if((errno == EAGAIN) ||
						(errno == EWOULDBLOCK))
						{
							break;
						}
						else
						{
							perror("accept");
							break;
						}
					}
					errHndl = getnameinfo(&in_addr, in_len,
									hbuf, sizeof(hbuf),
									sbuf, sizeof(sbuf),
									NI_NUMERICHOST | NI_NUMERICSERV);
					if(errHndl == 0)
					{
						printf("%s: Accepted connection on descriptor %d"
							"(host=%s, port=%s)\n", server->_serverName.c_str(), infd, hbuf, sbuf);
					}

					errHndl = make_socket_non_blocking(infd);
					if(errHndl == -1)
						abort();
					
					struct epoll_event  current_event;
					current_event.data.fd = infd;
					current_event.events = EPOLLIN | EPOLLET;
					errHndl = epoll_ctl(_epfd, EPOLL_CTL_ADD, infd, &current_event);
					if(errHndl == -1)
					{
						perror("epoll_ctl");
						abort();
					}
					fds.setFD(infd);
				}
			}
			else
			{
				bool done = false;
				while(1)
				{
					ssize_t count;
					char buf[512];

					count = read(currentEvent.data.fd, buf, sizeof(buf));
					if(count == -1)
					{
						if(errno != EAGAIN)
						{
							perror("read");
							done = true;
						}
						break;
					}
					else if(count == 0)
					{
						done = true;
						break;
					}
					if (strncmp(buf, "exit", 4) == 0){
						_isRunning = false;
						done = true;
					}
					buf[count] = '\n';
					errHndl = write(1, buf, count+1);
					if(errHndl == -1)
					{
						perror("write");
						abort();
					}

					errHndl = write(currentEvent.data.fd, buf, count+1);
					if(errHndl == -1)
					{
						perror("socket write");
						abort();
					}
				}
				if(done == true)
				{
					if (epoll_ctl(_epfd, EPOLL_CTL_DEL, currentEvent.data.fd, NULL) == -1)
					{
						perror("epoll_ctl: EPOLL_CTL_DEL");
					}
					printf("%s: Closed connection on descriptor %d\n", server->_serverName.c_str(), currentEvent.data.fd);
					fds.closeFD(currentEvent.data.fd);
				}
			}
		}
	}
}

int Server::runServers(ServerList& servers, FileDescriptor& fds)
{

    while (_isRunning == true)
    {
        int eventCount;

        fprintf(stdout, "Blocking and waiting for epoll event...\n");
        eventCount = epoll_wait(_epfd, _events.data(), FD_LIMIT, -1);
        // eventCount = epoll_wait(_epfd, _events.data(), FD_LIMIT, 5000);
        if (eventCount == -1) // for use only goes wrong with EINTR(signals)
        {
            std::cerr << "Server epoll_wait: " << strerror(errno);
            return -1;
        }
        fprintf(stdout, "Received epoll event\n");

		handleEvents(servers, fds, eventCount);
    }
    return 0;
}
