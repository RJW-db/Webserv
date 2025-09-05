#include <sys/epoll.h>
#include "ErrorCodeClientException.hpp"
#include "HandleTransfer.hpp"
#include "RunServer.hpp"
#include "Constants.hpp"
#include "Logger.hpp"

HandleWriteToCgiTransfer::HandleWriteToCgiTransfer(Client &client, const string &body, int fdWriteToCgi)
: HandleTransfer(client, fdWriteToCgi, HANDLE_WRITE_TO_CGI_TRANSFER), _bytesWrittenTotal(0)
{
    _fileBuffer = body;
    _handleType = HANDLE_WRITE_TO_CGI_TRANSFER;
}

bool HandleWriteToCgiTransfer::writeToCgiTransfer()
{
    ssize_t sent = write(_fd, _fileBuffer.data() + _bytesWrittenTotal, _fileBuffer.size() - _bytesWrittenTotal);
    if (sent == -1)
    {
        _client._cgiClosing = true;
        throw ErrorCodeClientException(_client, 500, "Writing to CGI failed");
    }
    else if (sent > 0)
    {
        _bytesWrittenTotal += static_cast<size_t>(sent);
        _client.setDisconnectTimeCgi(DISCONNECT_DELAY_SECONDS);
        if (_fileBuffer.size() == _bytesWrittenTotal)
        {
            FileDescriptor::cleanupEpollFd(_fd);
            return true;
        }
    }
    return false;
}

HandleReadFromCgiTransfer::HandleReadFromCgiTransfer(Client &client, int fdReadfromCgi)
: HandleTransfer(client, fdReadfromCgi, HANDLE_READ_FROM_CGI_TRANSFER)
{
    _handleType = HANDLE_READ_FROM_CGI_TRANSFER;
}

bool HandleReadFromCgiTransfer::readFromCgiTransfer()
{
    vector<char> buff(PIPE_BUFFER_SIZE);
    
    ssize_t bytesRead = read(_fd, buff.data(), buff.size());
    if (bytesRead == -1)
    {
        FileDescriptor::cleanupEpollFd(_fd);
        throw ErrorCodeClientException(_client, 500, "Reading from CGI failed: " + string(strerror(errno)));
    }
    size_t rd = static_cast<size_t>(bytesRead);
    _fileBuffer.append(buff.data(), rd);
    if (rd == 0 || buff[rd] == '\0')
    {
        FileDescriptor::cleanupEpollFd(_fd);
        _client.setDisconnectTimeCgi(DISCONNECT_DELAY_SECONDS);
        return true;
    }
    return false;
}
