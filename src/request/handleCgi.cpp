#include <sys/epoll.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include "ErrorCodeClientException.hpp"
#include "HttpRequest.hpp"
#include "RunServer.hpp"
#include "Constants.hpp"
#include "Client.hpp"
#include "Logger.hpp"
namespace
{
    constexpr int PARENT = 1;
    constexpr int CHILD = 0;

    void child(Client &client, int fdWriteToCgi[2], int fdReadfromCgi[2]);
    void setupCgiPipes(Client &client, int fdWriteToCgi[2], int fdReadfromCgi[2]);
    void setupChildProcess(Client &client, int fdWriteToCgi[2], int fdReadfromCgi[2]);
    void setupParentProcess(Client &client, string &body, int fdWriteToCgi[2], int fdReadfromCgi[2]);

    bool setPipeBufferSize(int pipeFd);
    void sigtermHandler(int signum);
    void setupChildPipes(int fdWriteToCgi[2], int fdReadfromCgi[2]);
    void closing_pipes(int fdWriteToCgi[2], int fdReadfromCgi[2]);
    bool endsWith(const string &str, const string &suffix);
    vector<string> createArgv(Client &client);
    vector<string> createEnvp(Client &client);
    vector<char *> convertToCharArray(const vector<string> &argvString);
}

bool HttpRequest::handleCgi(Client &client, string &body)
{
    int fdWriteToCgi[2] = { -1, -1 };   // Server → CGI (stdin)
    int fdReadfromCgi[2] = { -1, -1 };  // CGI → Server (stdout)

    setupCgiPipes(client, fdWriteToCgi, fdReadfromCgi);

    if (FileDescriptor::setNonBlocking(fdWriteToCgi[1]) == false ||
        FileDescriptor::setNonBlocking(fdReadfromCgi[0]) == false ||
        setPipeBufferSize(fdWriteToCgi[1]) == false ||
        setPipeBufferSize(fdReadfromCgi[0]) == false)
    {
        closing_pipes(fdWriteToCgi, fdReadfromCgi);
        throw ErrorCodeClientException(client, 500, "Failed to set fcntl for CGI handling");
    }

    client._pid = fork();
    if (client._pid == -1)
    {
        closing_pipes(fdWriteToCgi, fdReadfromCgi);
        exit(EXIT_FAILURE);
    }

    if (client._pid == CHILD)
        setupChildProcess(client, fdWriteToCgi, fdReadfromCgi);
    if (client._pid >= PARENT)
    {
        setupParentProcess(client, body, fdWriteToCgi, fdReadfromCgi);
        return true;
    }
    return false;
}

namespace
{
    void setupCgiPipes(Client &client, int fdWriteToCgi[2], int fdReadfromCgi[2])
    {
        if (pipe(fdWriteToCgi) == -1 || pipe(fdReadfromCgi) == -1)
        {
            closing_pipes(fdWriteToCgi, fdReadfromCgi);
            throw ErrorCodeClientException(client, 500, "Failed to create pipe(s) for CGI handling");
        }
        FileDescriptor::setFD(fdWriteToCgi[0]);
        Logger::log(CHILD_INFO, client, "Opened fdWriteToCgi[0]:", fdWriteToCgi[0]);
        FileDescriptor::setFD(fdWriteToCgi[1]);
        Logger::log(CHILD_INFO, client, "Opened fdWriteToCgi[1]:", fdWriteToCgi[1]);
        FileDescriptor::setFD(fdReadfromCgi[0]);
        Logger::log(CHILD_INFO, client, "Opened fdReadfromCgi[0]:", fdReadfromCgi[0]);
        FileDescriptor::setFD(fdReadfromCgi[1]);
        Logger::log(CHILD_INFO, client, "Opened fdReadfromCgi[1]:", fdReadfromCgi[1]);
    }

    void setupChildProcess(Client &client, int fdWriteToCgi[2], int fdReadfromCgi[2])
    {
        if (signal(SIGTERM, &sigtermHandler) == SIG_ERR)
            Logger::logExit(ERROR, "Signal handler error", '-', "SIGTERM failed", strerror(errno));
        Logger::log(CHILD_INFO, client, "child ._pid ", client._pid);
        Logger::log(CHILD_INFO, "we got into child");
        child(client, fdWriteToCgi, fdReadfromCgi);
    }

    void setupParentProcess(Client &client, string &body, int fdWriteToCgi[2], int fdReadfromCgi[2])
    {
        Logger::log(CHILD_INFO, client, "parent ._pid ", client._pid);

        FileDescriptor::closeFD(fdWriteToCgi[0]);
        FileDescriptor::closeFD(fdReadfromCgi[1]);

        unique_ptr<HandleTransfer> handleWrite = nullptr;
        unique_ptr<HandleTransfer> handleRead = nullptr;
        uint16_t errorCode = 500; 
        try
        {
            RunServers::setEpollEventsClient(client, fdReadfromCgi[0], EPOLL_CTL_ADD, EPOLLIN);
                    
            if (client._useMethod & METHOD_POST)
            {
                RunServers::setEpollEventsClient(client, fdWriteToCgi[1], EPOLL_CTL_ADD, EPOLLOUT);
                handleWrite = make_unique<HandleWriteToCgiTransfer>(client, body, fdWriteToCgi[1]);
                RunServers::insertHandleTransferCgi(move(handleWrite));
            }
            else
                FileDescriptor::closeFD(fdWriteToCgi[1]);
            handleRead = make_unique<HandleReadFromCgiTransfer>(client, fdReadfromCgi[0]);
            RunServers::insertHandleTransferCgi(move(handleRead));

            client.setDisconnectTimeCgi(DISCONNECT_DELAY_SECONDS);
            return;
        }
        catch (const ErrorCodeClientException &e)
        {
            Logger::log(ERROR, "CGI error", '-', "ErrorCodeClientException in parent process: ", e.what());
            errorCode = e.getErrorCode();
        }
        catch (const exception& e)
        {
            Logger::log(ERROR, "CGI error", '-', "Exception in parent process: ", e.what());
        }
        catch (...)
        {
            Logger::log(ERROR, "CGI error", '-', "Unknown exception caught in parent process");
        }

        FileDescriptor::cleanupEpollFd(fdWriteToCgi[1]);
        FileDescriptor::cleanupEpollFd(fdReadfromCgi[0]);
        if (handleWrite)
            handleWrite->_fd = -1;
        if (handleRead)
            handleRead->_fd = -1;
        throw ErrorCodeClientException(client, errorCode, "CGI setup failed");
    }
    /**
     * you can expand the pipe buffer, normally is 65536, using fcntl F_SETPIPE_SZ.
     * check the maximum size: cat /proc/sys/fs/pipe-max-size
     */
    bool setPipeBufferSize(int pipeFd)
    {
        const size_t pipeSize = PIPE_BUFFER_SIZE; // 512 KB

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

    void child(Client &client, int fdWriteToCgi[2], int fdReadfromCgi[2])
    {
        try
        {
            setupChildPipes(fdWriteToCgi, fdReadfromCgi);
            closing_pipes(fdWriteToCgi, fdReadfromCgi);

            vector<string> argvString = createArgv(client);
            vector<char *> argv = convertToCharArray(argvString);
            // printVecArray(argv);

            vector<string> envpString = createEnvp(client);
            vector<char *> envp = convertToCharArray(envpString);
            // printVecArray(envp);

            char *filePath = (argv[1] != NULL) ? argv[1] : argv[0];
            execve(filePath, argv.data(), envp.data());
            Logger::log(IWARN, client, "execve failed for CGI handling, filePath: ", filePath, " errno: ", strerror(errno));
        }
        catch (const exception &e)
        {
            Logger::log(ERROR, "CGI error", '-', "Exception in child process: ", e.what());
        }
        catch (...)
        {
            Logger::log(ERROR, "CGI error", '-', "Unknown exception caught in child process");
        }
        FileDescriptor::safeCloseFD(STDIN_FILENO);
        FileDescriptor::safeCloseFD(STDOUT_FILENO);
        exit(EXIT_FAILURE);
    }

    void setupChildPipes(int fdWriteToCgi[2], int fdReadfromCgi[2])
    {
        if (dup2(fdWriteToCgi[0], STDIN_FILENO) == -1)
        {
            closing_pipes(fdWriteToCgi, fdReadfromCgi);
            exit(EXIT_FAILURE);
        }
        if (dup2(fdReadfromCgi[1], STDOUT_FILENO) == -1)
        {
            closing_pipes(fdWriteToCgi, fdReadfromCgi);
            close(STDIN_FILENO);
            exit(EXIT_FAILURE);
        }
    }

    void closing_pipes(int fdWriteToCgi[2], int fdReadfromCgi[2])
    {
        FileDescriptor::closeFD(fdWriteToCgi[0]);
        FileDescriptor::closeFD(fdWriteToCgi[1]);
        FileDescriptor::closeFD(fdReadfromCgi[0]);
        FileDescriptor::closeFD(fdReadfromCgi[1]);
    }

    vector<string> createArgv(Client &client)
    {
        vector<string> argvString;
        if (!client._location.getCgiPath().empty())
        {
            Logger::log(WARN, client, "client._location.getCgiPath() ", client._location.getCgiPath());
            argvString.push_back(client._location.getCgiPath());
        }
        else if (endsWith(client._requestPath, ".py"))
            argvString.push_back("python3");
        else if (endsWith(client._requestPath, ".php"))
            argvString.push_back("php");

        argvString.push_back('.' + client._requestPath);
        return argvString;
    }

    inline bool endsWith(const string &str, const string &suffix)
    {
        return (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0);
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
            envpString.push_back("CONTENT_TYPE=" + contentType + "; boundary=" + string(client._boundary));

            if (!client._location.getUploadStore().empty())
                envpString.push_back("UPLOAD_STORE=" + client._location.getUploadStore());
            else
                envpString.push_back(string("UPLOAD_STORE=") + "./");
        }

        envpString.push_back("GATEWAY_INTERFACE=CGI/1.1");
        envpString.push_back("SERVER_NAME=" + client._usedServer->getServerName());
        envpString.push_back("SERVER_PORT=" + client._ipPort.second);

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

    // void printVecArray(vector<char *> &args)
    // {
    //     for (auto it = args.begin(); it != args.end() && *it != NULL; ++it)
    //     {
    //         cerr << *it << endl; // optional debug output
    //     }
    // }
}
