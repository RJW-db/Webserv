#include <sys/wait.h>
#ifdef __linux__
# include <sys/epoll.h>
#endif
#include "ErrorCodeClientException.hpp"
#include "HttpRequest.hpp"
#include "RunServer.hpp"
#include "Constants.hpp"
#include "Logger.hpp"

void RunServers::disconnectChecks()
{
    try {
        checkCgiDisconnect();
        checkClientDisconnects();
    }
    catch (ErrorCodeClientException &e) {
        e.handleErrorClient();
    }
    catch (Logger::ErrorLogExit&) {
        Logger::logExit(ERROR, "Server error", '-', "Restart now or finish existing clients and exit");
    }
    catch (const exception& e) {
        Logger::log(ERROR, "Server error", '-', "Exception in disconnectChecks: ", e.what());
    }
}

HandleTransferIter RunServers::cleanupHandleCgi(HandleTransferIter it, int clientfd)
{
    HandleTransferIter lastAfter = it;
    if (it != _handleCgi.begin())
        --it;
    while (it != _handleCgi.end()) {
        if ((*it)->_client._fd == clientfd)
        {
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
    for (auto it = _handleCgi.begin(); it != _handleCgi.end();) {
        int exit_code;
        Client &client = (*it)->_client;
        pid_t result = waitpid(client._pid, &exit_code, WNOHANG);
        if (result == 0) {
            if (client._disconnectTimeCgi <= chrono::steady_clock::now()) {
                client._cgiClosing = true;
                cleanupHandleCgi(it, client._fd);
                if (client._pid > 0)
                    kill(client._pid, SIGTERM);
                else
                    Logger::log(IERROR, "No valid PID to kill", client._fd, "clientFD", "Trying to kill pid ", client._pid); //testlog
                throw ErrorCodeClientException(client, 500, "Reading from CGI failed because it took too long");
                // continue;
            }
            ++it;
        }
        else if (result == -1) {
            Logger::log(ERROR, "Server error", '-', "waitpid failed for client ", client._fd, ": ", strerror(errno));
            it = cleanupHandleCgi(it, client._fd);
        }
        else {
            if (WIFEXITED(exit_code)) {
                Logger::log(INFO, "Child process exited ", client._fd, "Child", "exit code: ", WEXITSTATUS(exit_code));
                it = killCgiPipes(_handleCgi.begin(), client._pid);
            }

        }
    }
}

HandleTransferIter RunServers::killCgiPipes(HandleTransferIter it, pid_t pid)
{
    HandleTransferIter lastAfter = it;
    while (it != _handleCgi.end()) {
        try {
            if ((*it)->_client._pid == pid) {
                HandleTransfer &handle = *(*it);
                if (handle._handleType == HANDLE_READ_FROM_CGI_TRANSFER) {
                    while (handle._fd != -1)
                        if (handle.readFromCgiTransfer() == true)
                            break;
                    string clientResponse = HttpRequest::createResponseCgi(handle._client, handle._fileBuffer);
                    unique_ptr<HandleToClientTransfer> handleClient = make_unique<HandleToClientTransfer>(handle._client, clientResponse);
                    RunServers::insertHandleTransfer(move(handleClient));
                }
                it = _handleCgi.erase(it);
                lastAfter = it;
                continue;
            }
            ++it;
        }
        catch (ErrorCodeClientException& e) {
            e.handleErrorClient();
            return cleanupHandleCgi(it, pid);
        }
        catch (const exception& e) {
            Logger::log(ERROR, "Server error", '-', "Exception in cleanupHandleCgi: ", e.what());
            cleanupClient((*it)->_client);
            return _handleCgi.begin();
        }
    }
    return lastAfter;
}

void RunServers::checkClientDisconnects()
{
    try {
        for (auto it = _clients.begin(); it != _clients.end();) {
            unique_ptr<Client> &client = it->second;
            ++it;
            if (client->_disconnectTime <= chrono::steady_clock::now()) {
                client->_cgiClosing = true;
                checkCgiDisconnect();
                cleanupClient(*client);
            }
        }
    }
    catch (const exception &e) {
        Logger::log(ERROR, "Server error", '-', "Exception in checkClientDisconnects: ", e.what());
    }
}

void RunServers::removeHandlesWithFD(int fd)
{
    for(auto it = _handleCgi.begin(); it != _handleCgi.end(); ++it) {
        if ((*it)->_fd == fd) {
            if ((*it)->_handleType == HANDLE_READ_FROM_CGI_TRANSFER)
                FileDescriptor::cleanupEpollFd((*it)->_fd);
            else
                _handleCgi.erase(it);
            return;
        }
    }
    FileDescriptor::cleanupEpollFd(fd);
}

void RunServers::cleanupClient(Client &client)
{
    int clientFD = client._fd;
    Logger::log(INFO, client, "Disconnected");
    for (auto it = _handle.begin(); it != _handle.end();) {
        if ((*it)->_client._fd == clientFD)
            it = _handle.erase(it);
        else
            ++it;
    }
    cleanupHandleCgi(_handleCgi.begin(), clientFD);
    _clients.erase(clientFD);
    FileDescriptor::cleanupEpollFd(clientFD);
}
