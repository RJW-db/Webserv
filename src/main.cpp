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
	serverConfig[1] = (tmp_t){"Beta", "6789"};

    ServerList servers;
    size_t amount_servers = 2;
	for (size_t i = 0; i < amount_servers; ++i) {
		servers.push_back(std::make_unique<Server>(serverConfig + i));
	}

    // Run all servers
    Server::runServers(servers, fds);
    return 0;
}



// typedef union epoll_data {
// 	void        *ptr;
// 	int          fd;
// 	uint32_t     u32;
// 	uint64_t     u64;
// } epoll_data_t;

// struct epoll_event {
// 	uint32_t     events;      /* Epoll events */
// 	epoll_data_t data;        /* User data variable */
// };

int Server::runServers(ServerList& servers, FileDescriptor& fds)
{
    // int listener = create_listener_socket();
    // printf("listener %d\n", listener);
    // if (listener == -1)
    //     return -1;

    int                 epfd;
    struct epoll_event  current_event;
    struct epoll_event *events;

    epfd = epoll_create(FD_LIMIT); // parameter must be bigger than 0, rtfm
    if (epfd == -1)
    {
        std::cerr << "Server epoll_create: " << strerror(errno);
        return -1;
    }

	for (const std::unique_ptr<Server>& server : servers)
	{
		current_event.data.fd = server->_listener;
		current_event.events = EPOLLIN | EPOLLET;
		if (epoll_ctl(epfd, EPOLL_CTL_ADD, server->_listener, &current_event) == -1)
		{
			std::cerr << "Server epoll_ctl: " << strerror(errno) << std::endl;
			close(epfd);
			return -1;
		}
	}

    events = new epoll_event[64];
    if (events == NULL)
    {
        std::cerr << "Server new: " << strerror(errno);
        return -1;
    }
    int errHndl = 0;
    bool    client_said_quit_server = true;
    while (client_said_quit_server == true)
    {
        int n, i;

        fprintf(stdout, "Blocking and waiting for epoll event...\n");
        n = epoll_wait(epfd, events, 64, -1);
        // n = epoll_wait(epfd, events, 64, 5000);
        fprintf(stdout, "Received epoll event\n");
        if (n == -1) // for use only goes wrong with EINTR(signals)
        {
            std::cerr << "Server epoll_wait: " << strerror(errno);
            return -1;
        }
        for (i = 0; i < n; ++i)
        {
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (events[i].events & EPOLLIN) == 0)
            {
                fprintf(stderr, "epoll error\n");
                close(events[i].data.fd);
                continue;
            }

			for (const std::unique_ptr<Server>& server : servers)
			{
				if (server->_listener == events[i].data.fd)
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

						current_event.data.fd = infd;
						current_event.events = EPOLLIN | EPOLLET;
						errHndl = epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &current_event);
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

						count = read(events[i].data.fd, buf, sizeof(buf));
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
							client_said_quit_server = false;
							done = true;
						}
						buf[count] = '\n';
						errHndl = write(1, buf, count+1);
						if(errHndl == -1)
						{
							perror("write");
							abort();
						}

						errHndl = write(events[i].data.fd, buf, count+1);
						if(errHndl == -1)
						{
							perror("socket write");
							abort();
						}
					}
					if(done == true)
					{
						if (epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1)
						{
							perror("epoll_ctl: EPOLL_CTL_DEL");
						}
						printf("%s: Closed connection on descriptor %d\n", server->_serverName.c_str(), events[i].data.fd);
						fds.closeFD(events[i].data.fd);
					}
				}
			}
        }
    }
    close(epfd);
    delete[] events;
    return 0;
}
