#include <sys/epoll.h>
#include "RunServer.hpp"
#include "HandleTransfer.hpp"
#include "ErrorCodeClientException.hpp"

// HandleTransfer::HandleTransfer(Client &client, string &response)
// :_client(client), _fileBuffer(response)
// {

// }

HandleToClientTransfer::HandleToClientTransfer(Client &client, string &response)
: HandleTransfer(client, -1, HANDLE_TO_CLIENT_TRANSFER)
{
    _fileBuffer = response;
    _bytesReadTotal = 0;
    _handleType = HANDLE_TO_CLIENT_TRANSFER;
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLOUT);
}

bool HandleToClientTransfer::sendToClientTransfer()
{
    ssize_t sent = send(_client._fd, _fileBuffer.data() + _bytesReadTotal, _fileBuffer.size() - _bytesReadTotal, 0);
    if (sent <= 0)
    {
        if (sent == -1)
            throw ErrorCodeClientException(_client, 0, "Send to client went wrong: " + string(strerror(errno)));
        RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
        return true;
    }
    else if (sent > 0)
    {
        _bytesReadTotal += static_cast<size_t>(sent);
        if (_fileBuffer.size() == _bytesReadTotal)
        {
            RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
            return true;
        }
    }
    return false;
}

