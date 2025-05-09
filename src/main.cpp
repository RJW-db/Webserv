#include <Webserv.hpp>

#include <unistd.h>		// close
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>

#include <iostream>
void	remy_serv(void);


int main() {
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

void	remy_serv(void)
{
	struct addrinfo	serverSetup;
	struct addrinfo	*server;
	int				errHndl;

	std::memset(&serverSetup, 0, sizeof(serverSetup));
	serverSetup.ai_family = AF_UNSPEC;
	serverSetup.ai_socktype = SOCK_STREAM;
	serverSetup.ai_flags = AI_PASSIVE;
	errHndl = getaddrinfo(NULL, PORT, &serverSetup, &server);
	if (errHndl != 0) {
		std::cerr << "Server getaddrinfo: " << gai_strerror(errHndl);
		return;
	}
	
	struct addrinfo	*p;
	int				listener;
	for (p = server; server != NULL; p = p->ai_next) {
		listener = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
		if (listener < 0) {
			continue;
		}
		if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
			close(listener);
			continue;
		}

		break;
	}
	if (p == NULL) {
		std::cerr << "Server socket: " << strerror(errno);
		return;
	}
	puts("passed");
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