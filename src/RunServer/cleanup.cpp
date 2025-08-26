#ifdef __linux__
# include <sys/epoll.h>
#endif
#include <sys/wait.h>
#include <RunServer.hpp>
#include <HttpRequest.hpp>
#include <ErrorCodeClientException.hpp>
// #include <HttpRequest.hpp>
// #include <iostream>
// #include <FileDescriptor.hpp>
// #include <HandleTransfer.hpp>
#include "Logger.hpp"

void RunServers::disconnectChecks()
{
    try
    {
        checkCgiDisconnect();
        checkClientDisconnects();
    }
    catch (const ErrorCodeClientException &e)
    {
        e.handleErrorClient();  //TODO anything throwing in here stops the server
    }
    catch(Logger::ErrorLogExit&)
    {
        Logger::logExit(ERROR, "Server error", '-', "Restart now or finish existing clients and exit");
    }
    catch(const exception& e)
    {
        Logger::log(ERROR, "Server error", '-', "Exception in handleEvents: ", e.what());
    }
}

vector<std::unique_ptr<HandleTransfer>>::iterator RunServers::cleanupHandleCgi(vector<std::unique_ptr<HandleTransfer>>::iterator it, pid_t pid)
{
    vector<std::unique_ptr<HandleTransfer>>::iterator lastAfter = it;
    if (it != _handleCgi.begin())
        --it;
    while (it != _handleCgi.end())
    {
        if ((*it)->_client._pid == pid)
        {
            FileDescriptor::cleanupFD((*it)->_fd);
            it = _handleCgi.erase(it);
            lastAfter = it;
            continue ;
        }
        ++it;
    }
    return lastAfter;
}


void RunServers::checkCgiDisconnect()
{
    for (auto it = _handleCgi.begin(); it != _handleCgi.end();)
    {
        int exit_code;
        Client &client = (*it)->_client;

        // if (client._cgiClosing == true)
        // {
        //     Logger::log(DEBUG, +(*it)->_handleType, ", Child process for client ", client._fd, " has closed its pipes"); //testlog
        //     // FileDescriptor::cleanupFD((*it)->_fd);
        //     it = cleanupHandleCgi(it, client._pid);
        //     // it = _handleCgi.erase(it);
        //     continue ;
        // }
        // std::cout << "_handleType " << +((*it)->_handleType) << std::endl; //testcout
        pid_t result = waitpid(client._pid, &exit_code, WNOHANG);
        if (result == 0)
        {
            if (client._disconnectTimeCgi <= chrono::steady_clock::now())
            {
                client._cgiClosing = true;
                kill(client._pid, SIGTERM);
                Logger::log(DEBUG, "Killed child"); //testlog
                FileDescriptor::cleanupFD((*it)->_fd);
                it = cleanupHandleCgi(it, client._pid);
                // it = _handleCgi.erase(it);
                RunServers::setEpollEvents(client._fd, EPOLL_CTL_MOD, EPOLLIN);
                throw ErrorCodeClientException(client, 500, "Reading from CGI failed because it took too long");
                // continue;
            }
            ++it;
        }
        else
        {
            if (WIFEXITED(exit_code))
            {
                if (WEXITSTATUS(exit_code) > 0) // Client has already received the error code
                    throw ErrorCodeClientException(client, 500, "Cgi error exited with status: " + to_string(WEXITSTATUS(exit_code)));
                Logger::log(INFO, "cgi process with pid: ", client._pid, " exited with status: ", WEXITSTATUS(exit_code)); //testlog
                HandleTransfer& handle = *(*it);
                // Logger::log(DEBUG, "handletype: ", handle._handleType); //testlog
                if (handle._handleType == HANDLE_WRITE_TO_CGI_TRANSFER ||
                    handle._handleType == HANDLE_READ_FROM_CGI_TRANSFER)
                {
                    FileDescriptor::cleanupFD(handle._fd);
                    if (handle._handleType == HANDLE_READ_FROM_CGI_TRANSFER)
                    {
                        string clientResponse = HttpRequest::createResponseCgi(handle._client, handle._fileBuffer);
                        unique_ptr handleClient = make_unique<HandleToClientTransfer>(handle._client, clientResponse);
                        RunServers::insertHandleTransfer(move(handleClient));
                    }
                    client._cgiClosing = true;
                    Logger::log(DEBUG, +(*it)->_handleType, ", Child process for client ", client._fd, " has closed its pipes"); //testlog
                    it = cleanupHandleCgi(it, client._pid);
                    // it = _handleCgi.erase(it);
                    // continue ;
                }
                else
                {
                    ++it;
                }

            }

        }
    }
}

void RunServers::checkClientDisconnects()
{
    try
    {
        for (auto it = _clients.begin(); it != _clients.end();)
        {
            unique_ptr<Client> &client = it->second;
            ++it;
            if (client->_disconnectTime <= chrono::steady_clock::now())
            {
                // cout << "disconnectTime: "
                //         << chrono::duration_cast<chrono::milliseconds>(client->_disconnectTime.time_since_epoch()).count()
                //         << " ms" << endl;
                // cout << "now: "
                //         << chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count()
                //         << " ms" << endl;
                client->_cgiClosing = true;
                checkCgiDisconnect();
                cleanupClient(*client);
                Logger::log(DEBUG, "_handleCgi.size()  ", _handleCgi.size()); //testlog

            }
        }
    }
    catch (const std::exception &e)
    {
        Logger::log(ERROR, "Server error", '-', "Exception in checkClientDisconnects: ", e.what());
    }
}

void RunServers::removeHandlesWithFD(int fd)
{
    for(auto it = _handleCgi.begin(); it != _handleCgi.end(); ++it)
    {
        if ((*it)->_fd == fd)
        {
            _handleCgi.erase(it);
            return;
        }
    }
}

void RunServers::closeHandles(pid_t pid)
{
    for (auto it = _handleCgi.begin(); it != _handleCgi.end();)
    {
        if ((*it)->_client._pid == pid)
        {
            FileDescriptor::cleanupFD((*it)->_fd);
            it =_handleCgi.erase(it);
            continue ;
        }
        ++it;
    }
}

void RunServers::clientHttpCleanup(Client &client)
{
    client._headerParseState = HEADER_AWAITING;
    client._header.clear();
    client._body.clear();
    client._requestPath.clear();
    client._queryString.clear();
    client._method.clear();
    client._useMethod = 0;
    client._contentLength = 0;
    client._headerFields.clear();
    client._rootPath.clear();
	client._filenamePath.clear();
    client._name.clear();
    client._version.clear();
    client._bodyEnd = 0;
    client._filename.clear();
    client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    client._isAutoIndex = false;
    client._isCgi = false;
    // client._pid = 0;
}

void RunServers::cleanupClient(Client &client)
{
    int clientFD = client._fd;
    Logger::log(INFO, client, "Disconnected");
    for (auto it = _handle.begin(); it != _handle.end();)
    {
        if ((*it)->_client._fd == clientFD)
            it = _handle.erase(it);
        else
            ++it;
    }
    for (auto it = _handleCgi.begin(); it != _handleCgi.end();)
    {
        if ((*it)->_client._fd == clientFD)
            it = _handleCgi.erase(it);
        else
            ++it;
    }
    _clients.erase(clientFD);
    FileDescriptor::cleanupFD(clientFD);
}
