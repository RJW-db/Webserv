#include <unistd.h>
#include <sys/epoll.h>
#include <sys/wait.h>

#include <RunServer.hpp>
#include <Client.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>
#include <Logger.hpp>

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

void closing_pipes(int fdWriteToCgi[2], int fdReadfromCgi[2])
{
    FileDescriptor::closeFD(fdWriteToCgi[0]);
    FileDescriptor::closeFD(fdWriteToCgi[1]);
    FileDescriptor::closeFD(fdReadfromCgi[0]);
    FileDescriptor::closeFD(fdReadfromCgi[1]);
}

void setupChildPipes(Client &client, int fdWriteToCgi[2], int fdReadfromCgi[2])
{
    if (dup2(fdWriteToCgi[0], STDIN_FILENO) == -1)
    {
        closing_pipes(fdWriteToCgi, fdReadfromCgi);
        std::exit(1);
        throw ErrorCodeClientException(client, 500, "Failed to dup2 inpipe to stdin for CGI handling");
    }
    if (dup2(fdReadfromCgi[1], STDOUT_FILENO) == -1)
    {
        closing_pipes(fdWriteToCgi, fdReadfromCgi);
        close(STDIN_FILENO);
        std::exit(1);
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
        Logger::log(WARN, client, "client._location.getCgiPath() ", client._location.getCgiPath());
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
    return argvString;
}

vector<string> createEnvp(Client &client)
{
    vector<string> envpString;
    envpString.push_back("REQUEST_METHOD=" + client._method);
    envpString.push_back("CONTENT_LENGTH=" + to_string(client._contentLength));
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


        // std::cerr << "CONTENT_TYPE=Content-Type: " + contentType + "; boundary=" + string(client._bodyBoundary) << std::endl; //testcout

        // envpString.push_back("CONTENT_TYPE=" + contentType + "; boundary=" + string(client._bodyBoundary));
        // envpString.push_back("CONTENT_TYPE=Content-Type: " + contentType + "; boundary=...");
        // std::exit(0);
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
        cerr << *it << endl; // optional debug output
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
    char *filePath = (argv[1] != NULL) ? argv[1] : argv[0];
    execve(filePath, argv.data(), envp.data());
    perror("execve failed");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    std::exit(EXIT_FAILURE);
}

/**
 * you can expand the pipe buffer, normally is 65536, using fcntl F_SETPIPE_SZ.
 * check the maximum size: cat /proc/sys/fs/pipe-max-size
 */
bool setPipeBufferSize(int pipeFd)
{
    const size_t pipeSize = 500 * 1024; // 500 KB

    if (fcntl(pipeFd, F_SETPIPE_SZ, pipeSize) == -1)
    {
        Logger::log(ERROR, "CGI error", pipeFd, "fcntl (F_SETPIPE_SZ): ", strerror(errno));
        return false;
    }
    return true;
}

void sigtermHandler(int signum)
{
    if (signum == SIGTERM)
    {
        Logger::log(WARN, "CGI warning", "-", "SIGTERM, CGI timeout, exiting child process");
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        exit(EXIT_FAILURE);
    }
}

bool HttpRequest::handleCgi(Client &client, string &body)
{
    int fdWriteToCgi[2] = { -1, -1 };   // Server → CGI (stdin)
    int fdReadfromCgi[2] = { -1, -1 };  // CGI → Server (stdout)

    if (pipe(fdWriteToCgi) == -1 || pipe(fdReadfromCgi) == -1) {
        closing_pipes(fdWriteToCgi, fdReadfromCgi);
        throw ErrorCodeClientException(client, 500, "Failed to create pipe(s) for CGI handling");
    }
    FileDescriptor::setFD(fdWriteToCgi[0]);   // CGI reads from this (CGI's stdin)
    Logger::log(CHILD_INFO, client, "Opened fdWriteToCgi[0]:", fdWriteToCgi[0]);
    FileDescriptor::setFD(fdWriteToCgi[1]);   // Server writes to CGI's stdin
    Logger::log(CHILD_INFO, client, "Opened fdWriteToCgi[1]:", fdWriteToCgi[1]);
    FileDescriptor::setFD(fdReadfromCgi[0]);  // Server reads from this
    Logger::log(CHILD_INFO, client, "Opened fdReadfromCgi[0]:", fdReadfromCgi[0]);
    FileDescriptor::setFD(fdReadfromCgi[1]);  // CGI writes to this (CGI's stdout)
    Logger::log(CHILD_INFO, client, "Opened fdReadfromCgi[1]:", fdReadfromCgi[1]);

    if (FileDescriptor::setNonBlocking(fdWriteToCgi[1]) == false ||  // Server writes to CGI
        FileDescriptor::setNonBlocking(fdReadfromCgi[0]) == false || // Server reads from CGI
        setPipeBufferSize(fdWriteToCgi[1]) == false ||
        setPipeBufferSize(fdReadfromCgi[0]) == false)
    {
        closing_pipes(fdWriteToCgi, fdReadfromCgi);
        throw ErrorCodeClientException(client, 500, "Failed to set fcntl for CGI handling");
    }

    client._pid = fork();
    if (client._pid == -1) {
        closing_pipes(fdWriteToCgi, fdReadfromCgi);
        std::exit(1);
        throw ErrorCodeClientException(client, 500, "Failed to fork for CGI handling");
    }

    if (client._pid == CHILD)
    {
        if (signal(SIGTERM, &sigtermHandler) == SIG_ERR)
            Logger::logExit(ERROR, "Signal handler error", '-', "SIGTERM failed", strerror(errno));
        Logger::log(CHILD, client, "child ._pid ", client._pid); //testlog
        Logger::log(CHILD, "we got into child"); //testlog
        // sleep(10);
        child(client, fdWriteToCgi, fdReadfromCgi);
    }
    if (client._pid >= PARENT)
    {
        Logger::log(CHILD, client, "parent ._pid ", client._pid); //testlog

        FileDescriptor::closeFD(fdWriteToCgi[0]);
        FileDescriptor::closeFD(fdReadfromCgi[1]);
        
        // try
        // {
            unique_ptr<HandleTransfer> handle;
            if (client._useMethod & METHOD_POST)
            {
                handle = make_unique<HandleWriteToCgiTransfer>(client, body, fdWriteToCgi[1]);
                RunServers::insertHandleTransferCgi(move(handle));
            }

            handle = make_unique<HandleReadFromCgiTransfer>(client, fdReadfromCgi[0]);
            RunServers::insertHandleTransferCgi(move(handle));


            client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
        // }
        // catch(const std::exception& e)
        // {
        //     closing_pipes(fdWriteToCgi, fdReadfromCgi);
        //     kill(client._pid, SIGKILL);
        //     throw e;
        //     std::cerr << e.what() << '\n';
        // }
        client.setDisconnectTimeCgi(DISCONNECT_DELAY_SECONDS);
        
        RunServers::setEpollEvents(fdWriteToCgi[1], EPOLL_CTL_ADD, EPOLLOUT);
        RunServers::setEpollEvents(fdReadfromCgi[0], EPOLL_CTL_ADD, EPOLLIN);


        return true;
    }
    return false;
}
