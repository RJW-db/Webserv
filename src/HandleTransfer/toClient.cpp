#include <sys/socket.h>
#include <sys/epoll.h>
#include <cstring>
#include "ErrorCodeClientException.hpp"
#include "HandleTransfer.hpp"
#include "RunServer.hpp"
#include "Constants.hpp"
#include "Client.hpp"

HandleToClientTransfer::HandleToClientTransfer(Client &client, const string &response)
: HandleTransfer(client, -1, HANDLE_TO_CLIENT_TRANSFER)
{
    _fileBuffer = response;
    RunServers::setEpollEventsClient(client, _client._fd, EPOLL_CTL_MOD, EPOLLOUT);
}

bool HandleToClientTransfer::sendToClientTransfer()
{
    ssize_t sent = send(_client._fd, _fileBuffer.data() + _bytesReadTotal, _fileBuffer.size() - _bytesReadTotal, 0);
    if (sent >= 0) {
        _bytesReadTotal += static_cast<size_t>(sent);
        if (_fileBuffer.size() == _bytesReadTotal || sent == 0) {
            RunServers::setEpollEventsClient(_client, _client._fd, EPOLL_CTL_MOD, EPOLLIN);
            return true;
        }
    }
    else
        throw ErrorCodeClientException(_client, INFO_LOG, "Send to client went wrong: " + string(strerror(errno)));
    return false;
}
