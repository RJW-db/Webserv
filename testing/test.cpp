#include <string>
#include <string_view>
#include <iostream>

// int main()
// {
// 	std::string str = "Hello, World!";
// 	std::string_view view = str.substr(0, 5); // Extract "Hello"
// 	std::cout << view << std::endl;
// 	str.clear(); // Clear the original string
// 	std::cout << view << std::endl; // Still valid, prints "Hello"
// 	return 0;
// }

// int main()
// {
// 	std::string str = "Hello, World!";
// 	std::string_view view(str.data(), 5);
// 	std::cout << view << std::endl;
// 	str.clear(); // Clear the original string
// 	std::cout << view << std::endl; // view is freed, reading from it is undefined behavior
// 	return 0;
// }

#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int pipefd[2];
    pid_t cpid;
    char buf;

    if (argc != 2) {
		fprintf(stderr, "Usage: %s <string>\n", argv[0]);
		exit(EXIT_FAILURE);
    }
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    cpid = fork();
    if (cpid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (cpid == 0) {    /* Child reads from pipe */
        close(pipefd[1]);          /* Close unused write end */
        while (read(pipefd[0], &buf, 1) > 0)
            write(STDOUT_FILENO, &buf, 1);
        write(STDOUT_FILENO, "\n", 1);
        close(pipefd[0]);
        _exit(EXIT_SUCCESS);
    } else {            /* Parent writes argv[1] to pipe */
        close(pipefd[0]);          /* Close unused read end */
        write(pipefd[1], argv[1], strlen(argv[1]));
        close(pipefd[1]);          /* Reader will see EOF */
        wait(NULL);                /* Wait for child */
        exit(EXIT_SUCCESS);
    }
}