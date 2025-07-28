#include <unistd.h>
#include <sys/epoll.h>

#include <RunServer.hpp>
#include <Client.hpp>
#include <ErrorCodeClientException.hpp>

#define PASS_TO_CGI "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n" \
    "Content-Disposition: form-data; name=\"file\"; filename=\"chunk_test1.txt\"\r\n" \
    "Content-Type: text/plain\r\n" \
    "\r\n" \
    "This is the content of the file.\n" \
    "Second line of text.\n" \
    "third line of text\n" \
    "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n"

#define PARENT 1
#define CHILD 0


bool    safeCloseFD(int &fd)
{
    if (fd == -1)
        return true;
    
    int ret;
    do
    {
        ret = close(fd);
    } while (ret == -1 && errno == EINTR);

    if (ret == -1 && errno == EIO)
    {
        std::cerr << "No more new connenctions and finish up current connections and then restart webserver" << std::endl; //testcout
        return false;
    }

    fd = -1;
    return true;
}

void closing_pipes(int in_pipe[2], int out_pipe[2])
{
    safeCloseFD(in_pipe[0]);
    safeCloseFD(in_pipe[1]);
    safeCloseFD(out_pipe[0]);
    safeCloseFD(out_pipe[1]);
}

void setupChildPipes(Client &client, int in_pipe[2], int out_pipe[2])
{
    if (dup2(in_pipe[0], STDIN_FILENO) == -1) {
        closing_pipes(in_pipe, out_pipe);
        throw ErrorCodeClientException(client, 500, "Failed to dup2 inpipe to stdin for CGI handling");
    }
    if (dup2(out_pipe[1], STDOUT_FILENO) == -1)
    {
        closing_pipes(in_pipe, out_pipe);
        close(STDIN_FILENO);
        throw ErrorCodeClientException(client, 500, "Failed to dup2 outpipe to stdout for CGI handling");
    }
}

void child(Client &client, int in_pipe[2], int out_pipe[2])
{
    setupChildPipes(client, in_pipe, out_pipe);
    closing_pipes(in_pipe, out_pipe);

    // Set up args + env
    // char* argv[] = {"/usr/bin/python3", "script.py", NULL};
    // char* envp[] = {/* CGI env vars */};

    char* argv[] = {(char*)"./cgi", NULL};
    char* envp[] = {
        (char*)"REQUEST_METHOD=GET",
        (char*)"CONTENT_LENGTH=0",
        (char*)"QUERY_STRING=name=ChatGPT&lang=cpp",
        (char*)"SCRIPT_NAME=/cgi-bin/cgi",
        (char*)"SERVER_PROTOCOL=HTTP/1.1",
        (char*)"CONTENT_TYPE=text/plain",
        (char*)"GATEWAY_INTERFACE=CGI/1.1",
        NULL
    };

    // vector[asdsadsadadasdsa]
    // char *str[len(vector)]
    // for x in vector:
    //     str[x] = vector[x].data()

    // char **str = malloc(5);
    // str[0] = requestPath.data();

    execve(argv[0], argv, envp);
    perror("execve failed");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    exit(EXIT_FAILURE);
}

void handleCgi(Client &client)
{
    int in_pipe[2] = { -1, -1 };   // Server → CGI (stdin)
    int out_pipe[2] = { -1, -1 };  // CGI → Server (stdout)

    if (pipe(in_pipe) == -1 || pipe(out_pipe) == -1) {
        closing_pipes(in_pipe, out_pipe);
        throw ErrorCodeClientException(client, 500, "Failed to create pipe(s) for CGI handling");
    }

    if (RunServers::make_socket_non_blocking(in_pipe[1]) == false || // Server writes to CGI
        RunServers::make_socket_non_blocking(out_pipe[0]) == false) { // Server reads from CGI
        closing_pipes(in_pipe, out_pipe);
        throw ErrorCodeClientException(client, 500, "Failed to set fcntl for CGI handling");
    }

    pid_t pid = fork();
    if (pid == -1) {
        closing_pipes(in_pipe, out_pipe);
        throw ErrorCodeClientException(client, 500, "Failed to fork for CGI handling");
    }

    if (pid == CHILD) {
        child(client, in_pipe, out_pipe);
    }

    if (pid >= PARENT)
    {
        close(in_pipe[0]);    // parent doesn't read from stdin
        close(out_pipe[1]);   // parent doesn't write to stdout
        
        RunServers::setEpollEvents(out_pipe[0], EPOLL_CTL_ADD, EPOLLIN);
        RunServers::setEpollEvents(in_pipe[1], EPOLL_CTL_ADD, EPOLLOUT);

        write(in_pipe[1], PASS_TO_CGI, sizeof(PASS_TO_CGI) - 1);
        sleep(5);
        int ret;
        do
        {
            char buff[10];
            ret = read(out_pipe[0], buff, 10);
            write(1, buff, ret);
        } while (ret > 0);
        write(1, "\n", 1);
        
        // char buff[11];
        // if (read(pipefd[0], buff, 11) == -1)
        //     throw ErrorCodeClientException(client, 500, "Failed to read for CGI handling");
        // close in_pipe[1] when sending to cgi is done.
        // close out_pipe[0] when receiving from cgi is done.
        std::cout << "end of parent" << std::endl; //testcout
    }
}

// void handleCgi(Client &client)
// {
//     int pipefd[2];
//     pid_t pid;

//     if (pipe(pipefd) == -1)
//         throw ErrorCodeClientException(client, 500, "Failed to create pipe for CGI handling");
        
//     pid = fork();
//     if (pid == -1)
//         throw ErrorCodeClientException(client, 500, "Failed to fork for CGI handling");
    
//     // if (dup2(STDOUT_FILENO, pipefd[1]) == -1)
//     if (dup2(pipefd[1], STDOUT_FILENO) == -1)
//         throw ErrorCodeClientException(client, 500, "Failed to dup2 for CGI handling");
        
    
//     if (pid >= PARENT)
//     {
//         close(pipefd[0]);
//         write(STDOUT_FILENO, "Get in here", 11);
//         close(pipefd[1]);
//     }
//     if (pid == CHILD)
//     {
//         close(pipefd[1]);
//         char buff[11];
//         if (read(pipefd[0], buff, 11) == -1)
//             throw ErrorCodeClientException(client, 500, "Failed to read for CGI handling");

//         int fd = open("./newFile.txt", O_CREAT | O_WRONLY, 0644);
//         write(fd, buff, 11);
//         close(pipefd[0]);
//     }
// }