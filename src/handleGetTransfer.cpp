#include <HandleTransfer.hpp>
#include "Logger.hpp"
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <sys/epoll.h>

HandleGetTransfer::HandleGetTransfer(Client &client, int fd, string &responseHeader, size_t fileSize)
: HandleTransfer(client, fd, HANDLE_GET_TRANSFER), _fileSize(fileSize), _offset(0), _headerSize(responseHeader.size())
{
    _bytesReadTotal = 0;
    _fileBuffer = responseHeader;
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLOUT);
}

HandleGetTransfer &HandleGetTransfer::operator=(const HandleGetTransfer &other)
{
    if (this != &other) {
        // _client is a reference and cannot be assigned
        _fd = other._fd;
        _fileBuffer = other._fileBuffer;
        _fileSize = other._fileSize;
        _offset = other._offset;
        _bytesReadTotal = other._bytesReadTotal;
        _headerSize = other._headerSize;
    }
    return *this;
}
void HandleGetTransfer::readToBuf()
{
    if (_fd != -1)
    {
        char buff[RunServers::getClientBufferSize()];
        ssize_t bytesRead = read(_fd, buff, RunServers::getClientBufferSize());
        if (bytesRead == -1)
            throw RunServers::ClientException(string("handlingTransfer read: ") + strerror(errno) + ", fd: " + to_string(_fd) + ", on file: " + _client._filenamePath);
        size_t _bytesRead = static_cast<size_t>(bytesRead);
        _bytesReadTotal += _bytesRead;
        if (_bytesRead > 0)
        {
            _fileBuffer.append(buff, _bytesRead);
        }
        if (_bytesRead == 0 || _bytesReadTotal >= _fileSize)
        {
            FileDescriptor::closeFD(_fd);
            _fd = -1;
        }
    }
}

bool HandleGetTransfer::handleGetTransfer()
{
    readToBuf();
    ssize_t sent = send(_client._fd, _fileBuffer.c_str(), _fileBuffer.size(), MSG_NOSIGNAL);
    if (sent == -1)
    {
        throw ErrorCodeClientException(_client, 0, "send failed: " + string(strerror(errno)) + ", on file: " + _client._filenamePath);
    }
    size_t _sent = static_cast<size_t>(sent);
    _offset += _sent;
    _client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    if (_offset >= _fileSize + _headerSize) // TODO only between boundary is the filesize
    {
        RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
        Logger::log(INFO, _client, "GET    ", _client._filenamePath);
        return true;
    }
    _fileBuffer = _fileBuffer.erase(0, _sent);
    return false;
}
