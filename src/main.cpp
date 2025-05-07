#include <Webserv.hpp>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>


int main() {
	// poll_usages();
	// epoll_usage();
	// getaddrinfo_usage();
	server();
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