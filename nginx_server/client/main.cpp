#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/**
 * This file does the same thing as:
 * curl -v http://localhost:8080
 */
#define SERVER_PORT "8080"			// Port your Nginx server is running on
#define SERVER_ADDR "localhost"		// or, hostname -I <ip-address>

int main(void) {
	int sockfd;
	struct addrinfo hints, *res;
	// char request[] = "GET / HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\n\r\n";
	char request[] = "GET / HTTP/1.1\r\nHost: localhost:8080\r\nConnection: close\r\n\r\n";
	char buffer[1024];
	int bytes_received;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;  // IPv4
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(SERVER_ADDR, SERVER_PORT, &hints, &res) != 0) {
		perror("getaddrinfo");
		return 1;
	}

	// Create a socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		perror("socket");
		freeaddrinfo(res);
		return 1;
	}

	// Connect to the server
	if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		perror("connect");
		close(sockfd);
		freeaddrinfo(res);
		return 1;
	}

	// Send the GET request
	if (send(sockfd, request, strlen(request), 0) == -1) {
		perror("send");
		close(sockfd);
		freeaddrinfo(res);
		return 1;
	}

	// Receive the response and print it
	printf("Response from server:\n");
	while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
		buffer[bytes_received] = '\0';  // Null-terminate the buffer
		printf("%s", buffer);
	}

	if (bytes_received == -1) {
		perror("recv");
	}

	// Clean up and close the socket
	close(sockfd);
	freeaddrinfo(res);
	return 0;
}
