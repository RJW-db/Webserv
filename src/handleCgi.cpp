#include <unistd.h>
#include <sys/epoll.h>

#include <RunServer.hpp>
#include <Client.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>

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


// CGI
HandleWriteToCgiTransfer::HandleWriteToCgiTransfer(Client &client, string &fileBuffer, int fdWriteToCgi, int fdReadfromCgi)
: HandleTransfer(client, -1), _fdWriteToCgi(fdWriteToCgi), _fdReadfromCgi(fdReadfromCgi), _bytesWrittenTotal(0)
{
    _fileBuffer = fileBuffer;
    // _isCgi = writeToCgi;
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
}


void closing_pipes(int fdWriteToCgi[2], int fdReadfromCgi[2])
{
    FileDescriptor::closeFD(fdWriteToCgi[0]);
    FileDescriptor::closeFD(fdWriteToCgi[1]);
    FileDescriptor::closeFD(fdReadfromCgi[0]);
    FileDescriptor::closeFD(fdReadfromCgi[1]);
}

void setupChildPipes(Client &client, int fdWriteToCgi[2], int fdReadfromCgi[2])
{
    if (dup2(fdWriteToCgi[0], STDIN_FILENO) == -1) {
        closing_pipes(fdWriteToCgi, fdReadfromCgi);
        throw ErrorCodeClientException(client, 500, "Failed to dup2 inpipe to stdin for CGI handling");
    }
    if (dup2(fdReadfromCgi[1], STDOUT_FILENO) == -1)
    {
        closing_pipes(fdWriteToCgi, fdReadfromCgi);
        close(STDIN_FILENO);
        throw ErrorCodeClientException(client, 500, "Failed to dup2 outpipe to stdout for CGI handling");
    }
}

bool endsWith(const string &str, const string &suffix)
{
    return (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0);
}

vector<string> createArgv(Client &client)
{
    // argv[0] = "/usr/bin/python3" or "python3"
    // argv[1] = "./cgi-bin/upload.py"

    vector<string> argvString;
    if (!client._location.getCgiPath().empty())
    {
        std::cout << "client._location.getCgiPath() " << client._location.getCgiPath() << std::endl; //testcout
        argvString.push_back(client._location.getCgiPath());
    }
    else if (endsWith(client._requestPath, ".py"))
    {
        argvString.push_back("python3");
    } else if (endsWith(client._requestPath, ".php"))
    {
        argvString.push_back("php");
    }

    argvString.push_back('.' + client._requestPath);
    std::cout << "client._requestPath " << '.' + client._requestPath << std::endl; //testcout
    return argvString;
}

vector<string> createEnvp(Client &client)
{
    vector<string> envpString;
    envpString.push_back("REQUEST_METHOD=" + client._method);
    envpString.push_back("CONTENT_LENGTH=" + to_string(client._body.size()));
    if (!client._queryString.empty())
        envpString.push_back("QUERY_STRING=" + client._queryString);    // test http://localhost:8080/cgi-bin/cgi.py?WORK=YUR
    envpString.push_back("SCRIPT_NAME=" + client._requestPath);
    envpString.push_back("SERVER_PROTOCOL=" + client._version);
    if (client._useMethod & METHOD_POST)    /// cgi expects "Content-Type: multipart/form-data; boundary=------someboundary"
    {
        string contentType = string(client._contentType);
        // envpString.push_back("CONTENT_TYPE=" + string(contentType));
        // if (contentType == "multipart/form-data")
        //     envpString.push_back("HTTP_CONTENT_BOUNDARY=" + contentType);
        // if (contentType == "multipart/form-data")
        // envpString.push_back("CONTENT_TYPE=Content-Type: " + contentType + "; boundary=" + string(client._bodyBoundary));
        envpString.push_back("CONTENT_TYPE=" + contentType + "; boundary=" + string(client._bodyBoundary));

        if (!client._location.getUploadStore().empty())
        {
            envpString.push_back("UPLOAD_STORE=" + client._location.getUploadStore());
        }
        else
        {
            envpString.push_back(string("UPLOAD_STORE=") + "./");
        }
        

        // std::cout << "CONTENT_TYPE=Content-Type: " + contentType + "; boundary=" + string(client._bodyBoundary) << std::endl; //testcout

        // envpString.push_back("CONTENT_TYPE=" + contentType + "; boundary=" + string(client._bodyBoundary));
        // envpString.push_back("CONTENT_TYPE=Content-Type: " + contentType + "; boundary=...");
        // exit(0);
    }
    
    envpString.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envpString.push_back("SERVER_NAME=" + client._usedServer->getServerName());
    // envpString.push_back("SERVER_PORT=" + client._usedServer->getPortHost());

    envpString.push_back("DOCUMENT_ROOT=" + client._location.getRoot());
    return envpString;
}

vector<char *> convertToCharArray(const vector<string> &argvString)
{
    vector<char *> argv;
    for (const auto &it : argvString)
    {
        argv.push_back(const_cast<char *>(it.c_str()));
    }
    argv.push_back(NULL); // null-terminate for execve
    return argv;
}

void printVecArray(vector<char *> &args)
{
    for (auto it = args.begin(); it != args.end() && *it != NULL; ++it)
    {
        cout << *it << endl; // optional debug output
    }
}

void child(Client &client, int fdWriteToCgi[2], int fdReadfromCgi[2])
{
    setupChildPipes(client, fdWriteToCgi, fdReadfromCgi);
    closing_pipes(fdWriteToCgi, fdReadfromCgi);

    vector<string> argvString = createArgv(client);
    vector<char *> argv = convertToCharArray(argvString);
    // printVecArray(argv);

    vector<string> envpString = createEnvp(client);
    vector<char *> envp= convertToCharArray(envpString);
    // printVecArray(envp);

// exit(0);
    char *filePath = (argv[1] != NULL) ? argv[1] : argv[0];
    execve(filePath, argv.data(), envp.data());
    perror("execve failed");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    exit(EXIT_FAILURE);
}

bool HttpRequest::handleCgi(Client &client)
{
    int fdWriteToCgi[2] = { -1, -1 };   // Server → CGI (stdin)
    int fdReadfromCgi[2] = { -1, -1 };  // CGI → Server (stdout)

    if (pipe(fdWriteToCgi) == -1 || pipe(fdReadfromCgi) == -1) {
        closing_pipes(fdWriteToCgi, fdReadfromCgi);
        throw ErrorCodeClientException(client, 500, "Failed to create pipe(s) for CGI handling");
    }

    FileDescriptor::setFD(fdWriteToCgi[0]);   // CGI reads from this (CGI's stdin)
    FileDescriptor::setFD(fdWriteToCgi[1]);   // Server writes to CGI's stdin
    FileDescriptor::setFD(fdReadfromCgi[0]);  // Server reads from this
    FileDescriptor::setFD(fdReadfromCgi[1]);  // CGI writes to this (CGI's stdout)

    if (RunServers::make_socket_non_blocking(fdWriteToCgi[1]) == false || // Server writes to CGI
        RunServers::make_socket_non_blocking(fdReadfromCgi[0]) == false) { // Server reads from CGI
        closing_pipes(fdWriteToCgi, fdReadfromCgi);
        throw ErrorCodeClientException(client, 500, "Failed to set fcntl for CGI handling");
    }


    // int fd = open(client._filenamePath.data(), R_OK);
    // if (fd == -1)
    //     throw RunServers::ClientException("open failed on file: " + client._filenamePath + ", because: " + string(strerror(errno)));

    // // FileDescriptor::setFD(fd);
    // size_t fileSize = getFileLength(client._filenamePath);
    // string responseStr = HttpResponse(client, 200, client._filenamePath, fileSize);
    // auto handle = make_unique<HandleTransfer>(client, fd, responseStr, static_cast<size_t>(fileSize));
    // RunServers::insertHandleTransfer(move(handle));

    pid_t pid = fork();
    if (pid == -1) {
        closing_pipes(fdWriteToCgi, fdReadfromCgi);
        throw ErrorCodeClientException(client, 500, "Failed to fork for CGI handling");
    }

    if (pid == CHILD) {
        // size_t pos = client._rootPath.find_last_of('/');
        // string dirPath = client._rootPath.substr(0, pos);
        // std::cout << "dirpath: " << dirPath << std::endl; //testcout
        // std::cout << "client._uploadPath: " << client._location.getUploadStore() << std::endl; //testcout

        // std::cout << "cgi_path: " << client._location.getCgiPath() << std::endl; //testcout
        // // chdir(dirPath.data());
        // chdir(client._location.getUploadStore().c_str());
        child(client, fdWriteToCgi, fdReadfromCgi);


    }

    if (pid >= PARENT)
    {
        // exit(0);
        FileDescriptor::closeFD(fdWriteToCgi[0]);
        FileDescriptor::closeFD(fdReadfromCgi[1]);
        
        RunServers::setEpollEvents(fdWriteToCgi[1], EPOLL_CTL_ADD, EPOLLOUT);
        RunServers::setEpollEvents(fdReadfromCgi[0], EPOLL_CTL_ADD, EPOLLIN);

        unique_ptr<HandleWriteToCgiTransfer> handle;
        handle = make_unique<HandleWriteToCgiTransfer>(client, client._body, fdWriteToCgi[1], fdReadfromCgi[0]);
        handle->_client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
        if (handle->handleCgiTransfer() == true)
        {
            RunServers::clientHttpCleanup(client);
            return true;
        }
        RunServers::insertHandleTransfer(move(handle));


        // std::cout << client._body.size() << std::endl; //testcout
        // // int total_sent = write(fdWriteToCgi[1], client._body.data(), client._body.size());
        // size_t total_sent = 0;
        // /**
        //  * you can expand the pipe buffer, normally is 65536, using fcntl F_SETPIPE_SZ.
        //  * check the maximum size: cat /proc/sys/fs/pipe-max-size
        //  */
        // while (total_sent < client._body.size()) { 
        //     ssize_t sent = write(fdWriteToCgi[1], client._body.data() + total_sent, client._body.size() - total_sent);
        //     // ssize_t sent = write(fdWriteToCgi[1], client._body.data() + total_sent, 15000);
        //     if (sent == -1) {
        //         perror("write to CGI failed");
        //         break;
        //     }
        //     std::cout << "sent: " << sent << std::endl; //testcout
        //     sleep(1);
        //     total_sent += sent;
        // }

        // FileDescriptor::closeFD(fdWriteToCgi[1]);

        // std::cout << "sent: " << total_sent << std::endl; //testcout
        // sleep(2);
        // // write(fdWriteToCgi[1], PASS_TO_CGI, sizeof(PASS_TO_CGI) - 1);
        // // sleep(5);
        // int ret;
        // do
        // {
        //     char buff[10];
        //     ret = read(fdReadfromCgi[0], buff, 10);
        //     write(1, buff, ret);
        // } while (ret > 0);
        // write(1, "\n", 1);
        

        std::cout << "end of parent" << std::endl; //testcout
    }
    return false;
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