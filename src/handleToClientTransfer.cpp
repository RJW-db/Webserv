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
}

bool HandleToClientTransfer::sendToClientTransfer()
{
    ssize_t sent = send(_client._fd, _fileBuffer.data(), _fileBuffer.size() - _bytesReadTotal, 0);
    if (sent <= 0)
    {
        if (sent == -1)
            throw ErrorCodeClientException(_client, 500, "Reading from CGI failed: " + string(strerror(errno)));
        std::cout << "EOF reached, closing CGI read pipe" << std::endl; //testcout
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
        // _client.setDisconnectTime(RunServers::DISCONNECT_DELAY_SECONDS);
        std::cout << "sent " << sent << std::endl; //testcout
    }
    // std::cout << "_FD = " << _fd << std::endl; //testcout
    return false;
}

