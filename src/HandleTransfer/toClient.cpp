#include <sys/epoll.h>
#include <sys/socket.h>
#include <cstring>
#include "RunServer.hpp"
#include "HandleTransfer.hpp"
#include "ErrorCodeClientException.hpp"


HandleToClientTransfer::HandleToClientTransfer(Client &client, string &response)
: HandleTransfer(client, -1, HANDLE_TO_CLIENT_TRANSFER)
{
    _fileBuffer = response;
    _bytesReadTotal = 0;
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLOUT);
}

bool HandleToClientTransfer::sendToClientTransfer()
{
    ssize_t sent = send(_client._fd, _fileBuffer.data() + _bytesReadTotal, _fileBuffer.size() - _bytesReadTotal, 0);
    if (sent >= 0)
    {
        _bytesReadTotal += static_cast<size_t>(sent);
        if (_fileBuffer.size() == _bytesReadTotal || sent == 0)
        {
            RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
            return true;
        }
    }
    else
        throw ErrorCodeClientException(_client, 0, "Send to client went wrong: " + string(strerror(errno)));
    return false;
}
