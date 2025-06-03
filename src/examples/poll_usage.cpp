// #include <RunServer.hpp>

// #include <unistd.h>	// read
// #include <string.h>    // for strncmp
// #include <sys/poll.h>
// #include <sys/epoll.h>
// #include <iostream>
// #include <stdio.h>

// #define TIMEOUT 5

// /**
//  * poll only exists to check events on the fd that you connected them with, e.g.
//  * fd STDIN event POLLIN
//  */
// void	poll_usages(void)
// {
// 	struct pollfd fds[2];

// 	/* Watch stdin for input */
// 	// fds[0].fd = open("sam.txt", O_RDONLY, O_WRONLY, O_APPEND);
// 	fds[0].fd = STDIN_FILENO;
// 	fds[0].events = POLLIN;

// 	/* Watch stdout for ability to write */
// 	fds[1].fd = STDOUT_FILENO;
// 	fds[1].events = POLLOUT;

// 	//	created 2 fd
// 	nfds_t nfds = 2;
// 	while (true) {
// 		// int ret = poll(fds, 1, TIMEOUT * 1000); // Wait for events
// 		int ret = poll(fds, nfds, TIMEOUT * 1000); // Wait for events

// 		if (ret == -1) {
// 			perror("poll");
// 			return ;
// 		}

// 		if (ret == 0) {
// 			std::cout << TIMEOUT << " seconds elapsed. No events detected." << std::endl;
// 			// std::cerr << "ever here?" << std::endl;
// 			continue;
// 		}

// 		if (fds[0].revents  &POLLIN) {
// 			std::cout << "stdin is readable" << std::endl;
// 			break;
// 		}

// 		if (fds[1].revents  &POLLOUT) {
// 			std::cout << "stdout is writable" << std::endl;
// 			// sleep(3);
			
// 			// close(1);
// 			// fds[1].fd = -1;
// 		}
// 	}
// }


// #define MAX_EVENTS 5
// #define READ_SIZE 10
// void	epoll_usage(void)
// {
// 	int running = 1, event_count, i;
// 	ssize_t bytes_read;
// 	char read_buffer[READ_SIZE + 1];
// 	struct epoll_event event;
// 	struct epoll_event events[MAX_EVENTS];
// 	int epoll_fd = epoll_create1(0);

// 	if (epoll_fd == -1) {
// 		fprintf(stderr, "Failed to create epoll file descriptor\n");
// 		return;
// 	}

// 	event.events = EPOLLIN;
// 	event.data.fd = STDIN_FILENO;

// 	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &event))
// 	{
// 		fprintf(stderr, "Failed to add file descriptor to epoll\n");
// 		close(epoll_fd);
// 		return;
// 	}

// 	while (running) {
// 		printf("\nPolling for input...\n");
// 		event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 30000);
// 		printf("%d ready events\n", event_count);
// 		for (i = 0; i < event_count; i++) {
// 			printf("Reading file descriptor '%d' -- ", events[i].data.fd);
// 			bytes_read = read(events[i].data.fd, read_buffer, READ_SIZE);
// 			printf("%zd bytes read.\n", bytes_read);
// 			read_buffer[bytes_read] = '\0';
// 			printf("Read '%s'\n", read_buffer);
		
// 			if(!strncmp(read_buffer, "stop\n", 5))
// 			running = 0;
// 		}
// 	}

// 	if (close(epoll_fd)) {
// 		fprintf(stderr, "Failed to close epoll file descriptor\n");
// 		return;
// 	}

// 	puts("end of epoll_usage");
// 	return;
// }
