#include <Webserv.hpp>

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // close
#ifdef __linux__
#include <sys/epoll.h>
#endif

#include <iostream>

#define RESERVED_FDS 3         // stdin, stdout, stderr
#define INCOMING_FD_LIMIT 1024 // Makefile, shell(ulimit -n)
// TODO if FD minimum = 5, immediate turning off webserv with error.
// 4th is listener FD, 5th FD is client.
#define FD_LIMIT 1024 - 3

int              remy_serv(void);
int              create_listener_socket(void);
struct addrinfo *get_server_addrinfo(void);
int              bind_to_socket(struct addrinfo *server);
static int       make_socket_non_blocking(int sfd);

int main()
{
    // poll_usages();
    // epoll_usage();
    // getaddrinfo_usage();
    // server();
    remy_serv();
    return 0;
}

#define PORT "8080"
// getaddrinfo();
// socket();
// bind();
// listen();
// /* accept() goes here */

int remy_serv(void)
{
    struct pollfd *pfds;
    int            listener = create_listener_socket();
    if (listener == -1)
        return -1;

    // struct pollfd *pfds = (pollfd *)malloc(sizeof(pollfd) * FD_LIMIT);
    // if (pfds == NULL)
    // {
    //     close(listener);
    //     return -1;
    // }
    // int nfds = 0;
(void)pfds;
    puts("yur");
    if (epoll_create(FD_LIMIT) == -1) // parameter must be bigger than 0, rtfm
    {
        std::cerr << "Server listen: " << strerror(errno);
    	return -1;
    }
	return (0);
}

int create_listener_socket(void)
{
    struct addrinfo *server = get_server_addrinfo();
    if (server == NULL)
    {
        return -1;
    }

    int listener = bind_to_socket(server);
    freeaddrinfo(server);
    if (listener == -1)
    {
        return -1;
    }
    if (make_socket_non_blocking(listener) == -1)
    {
        return -1;
    }
    // TODO test with 1~5 maximum pending queue of people connecting
    if (listen(listener, SOMAXCONN) == -1)
    {
        std::cerr << "Server listen: " << strerror(errno);
        return -1;
    }
    return listener;
}

struct addrinfo *get_server_addrinfo(void)
{
    struct addrinfo  serverSetup;
    struct addrinfo *server;
    int              errHndl;

    std::memset(&serverSetup, 0, sizeof(serverSetup));
    serverSetup.ai_family = AF_UNSPEC;     // Use IPv4 or IPv6
    serverSetup.ai_socktype = SOCK_STREAM; // TCP stream sockets
    serverSetup.ai_flags = AI_PASSIVE;     // Use my IP

    errHndl = getaddrinfo(NULL, PORT, &serverSetup, &server);
    if (errHndl != 0)
    {
        std::cerr << "Server getaddrinfo: " << gai_strerror(errHndl)
                  << std::endl;
        return NULL;
    }
    return server;
}

int bind_to_socket(struct addrinfo *server)
{
    struct addrinfo *p;
    int              listener;
    for (p = server; server != NULL; p = p->ai_next)
    {
        listener =
            socket(server->ai_family, server->ai_socktype, server->ai_protocol);
        if (listener < 0)
        {
            continue;
        }
        int reUse = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reUse, sizeof(int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(listener);
            continue;
        }
        return listener;
    }
    std::cerr << "Server bind_to_socket: " << strerror(errno);
    return -1;
}

static int make_socket_non_blocking(int sfd)
{
    int currentFlags = fcntl(sfd, F_GETFL, 0);
    if (currentFlags == -1)
    {
        std::cerr << "fcntl: " << strerror(errno);
        return -1;
    }

    currentFlags |= O_NONBLOCK;
    int fcntlResult = fcntl(sfd, F_SETFL, currentFlags);
    if (fcntlResult == -1)
    {
        std::cerr << "fcntl: " << strerror(errno);
        return -1;
    }
    return 0;
}

// int nbytes = recv(pfds[i].fd, buf, sizeof buf - 1, 0);
// if (nbytes > 0) {
// 	buf[nbytes] = '\0'; // Null-terminate the buffer
// 	printf("Received request:\n%s\n", buf);

// 	// Parse the HTTP request (very basic parsing)
// 	if (strncmp(buf, "GET ", 4) == 0) {
// 		// Extract the requested path
// 		char *path = buf + 4;
// 		char *end_path = strchr(path, ' ');
// 		if (end_path) {
// 			*end_path = '\0'; // Null-terminate the path
// 			printf("Requested path: %s\n", path);
// 		}

// 		// Respond to the client
// 		const char *response =
// 			"HTTP/1.1 200 OK\r\n"
// 			"Content-Type: text/html\r\n"
// 			"Content-Length: 13\r\n"
// 			"\r\n"
// 			"Hello, world!";
// 		send(pfds[i].fd, response, strlen(response), 0);
// 	} else {
// 		// Handle unsupported methods
// 		const char *response =
// 			"HTTP/1.1 405 Method Not Allowed\r\n"
// 			"Content-Length: 0\r\n"
// 			"\r\n";
// 		send(pfds[i].fd, response, strlen(response), 0);
// 	}
// }