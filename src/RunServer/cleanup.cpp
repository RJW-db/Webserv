#ifdef __linux__
# include <sys/epoll.h>
#endif
#include <sys/wait.h>
#include <RunServer.hpp>
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

void RunServers::checkCgiDisconnect()
{
    for (auto it = _handleCgi.begin(); it != _handleCgi.end();)
    {
        int exit_code;
        Client &client = (*it)->_client;

        if (client._cgiClosing == true)
        {
            Logger::log(DEBUG, +(*it)->_handleType, ", Child process for client ", client._fd, " has closed its pipes"); //testlog
            FileDescriptor::cleanupFD((*it)->_fd);
            it = _handleCgi.erase(it);
            continue ;
        }
        // std::cout << "_handleType " << +((*it)->_handleType) << std::endl; //testcout
        pid_t result = waitpid(client._pid, &exit_code, WNOHANG);
        // Logger::log(DEBUG, "result: ", result); //testlog
        if (result == 0)
        {
            if (client._disconnectTimeCgi <= chrono::steady_clock::now())
            {
                client._cgiClosing = true;
                kill(client._pid, SIGTERM);
                Logger::log(DEBUG, "Killed child"); //testlog
                FileDescriptor::cleanupFD((*it)->_fd);
                it = _handleCgi.erase(it);
                RunServers::setEpollEvents(client._fd, EPOLL_CTL_MOD, EPOLLIN);
                throw ErrorCodeClientException(client, 500, "Reading from CGI failed");
                // continue;
            }
            ++it;
        }
        else
        {
            if (WIFEXITED(exit_code))
            {
                Logger::log(INFO, "cgi process with pid: ", client._pid, " exited with status: ", WEXITSTATUS(exit_code)); //testlog
                HandleTransfer& currentTransfer = *(*it);
                // Logger::log(DEBUG, "handletype: ", currentTransfer._handleType); //testlog
                if (currentTransfer._handleType == HANDLE_WRITE_TO_CGI_TRANSFER ||
                    currentTransfer._handleType == HANDLE_READ_FROM_CGI_TRANSFER)
                {
                    FileDescriptor::cleanupFD(currentTransfer._fd);
                    client._cgiClosing = true;
                    Logger::log(DEBUG, +(*it)->_handleType, ", Child process for client ", client._fd, " has closed its pipes"); //testlog
                    it = _handleCgi.erase(it);
                    continue ;
                }
                if (WEXITSTATUS(exit_code) > 0)
                    throw ErrorCodeClientException(client, 500, "Cgi error");
                // bool finished = currentTransfer.readFromCgiTransfer();
                // if (finished)
                // {
                //     it = _handleCgi.erase(it);
                //     if (client._keepAlive == false)
                //         cleanupClient(client);
                //     else
                //         clientHttpCleanup(client);
                // }
                // else
                //     ++it;
            }
            else
            {
                ++it;
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
