#include <sys/epoll.h>
#include <sys/socket.h>
#include "ErrorCodeClientException.hpp"
#include "HandleTransfer.hpp"
#include "RunServer.hpp"
#include "Constants.hpp"
#include "Logger.hpp"

HandleGetTransfer::HandleGetTransfer(Client &client, int fd, string &responseHeader, size_t fileSize, size_t headerSize)
: HandleTransfer(client, fd, HANDLE_GET_TRANSFER), _fileSize(fileSize), _headerSize(headerSize), _offset(0)
{
    _fileBuffer = responseHeader;
    RunServers::setEpollEventsClient(client, _client._fd, EPOLL_CTL_MOD, EPOLLOUT);
}

bool HandleGetTransfer::handleGetTransfer()
{
    fileReadToBuff();
    ssize_t sent = send(_client._fd, _fileBuffer.data() + _offset, _fileBuffer.size() - _offset, MSG_NOSIGNAL);
    if (sent == -1)
        throw ErrorCodeClientException(_client, 0, "send failed: " + string(strerror(errno)) + ", on file: " + _client._filenamePath);
    size_t _sent = static_cast<size_t>(sent);
    _offset += _sent;
    _bytesSentTotal += _sent;
    _client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    if (_bytesSentTotal >= _fileSize + _headerSize) {
        RunServers::setEpollEventsClient(_client, _client._fd, EPOLL_CTL_MOD, EPOLLIN);
        Logger::log(INFO, _client, "GET    ", _client._filenamePath);
        return true;
    }
    if (_fileBuffer.size() > RunServers::getRamBufferLimit()) {
        _fileBuffer = _fileBuffer.erase(0, _offset);
        _offset = 0;
    }
    return false;
}

void HandleGetTransfer::fileReadToBuff()
{
    if (_fd != -1) {
        char buff[CLIENT_BUFFER_SIZE];
        ssize_t bytesRead = read(_fd, buff, CLIENT_BUFFER_SIZE);
        if (bytesRead == -1)
            throw ErrorCodeClientException(_client, 500, "read on file: " + _client._filenamePath + ", failed because" + string(strerror(errno))); //TODO check if correct in case it can't read file for errorpage 500
        size_t rd = static_cast<size_t>(bytesRead);
        _bytesReadTotal += rd;
        if (rd > 0)
            _fileBuffer.append(buff, rd);
        else if (_bytesReadTotal >= _fileSize)
            FileDescriptor::closeFD(_fd);
    }
}
