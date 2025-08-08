#include <HandleTransfer.hpp>
#include <RunServer.hpp>
#include <sys/epoll.h>

HandleGet::HandleGet(Client &client, int fd, string &responseHeader, size_t fileSize)
: HandleShort(client, fd),  _fileSize(fileSize), _offset(0), _headerSize(responseHeader.size())
{
    _bytesReadTotal = 0;
    _fileBuffer = responseHeader;
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLOUT);
}

HandleGet &HandleGet::operator=(const HandleGet &other)
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
void HandleGet::readToBuf()
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

bool HandleGet::handleGetTransfer()
{
    readToBuf();
    ssize_t sent = send(_client._fd, _fileBuffer.c_str(), _fileBuffer.size(), MSG_NOSIGNAL);
    if (sent == -1)
        throw RunServers::ClientException(string("handlingTransfer send: ") + strerror(errno)); // TODO throw out client and remove handleTransfer
    size_t _sent = static_cast<size_t>(sent);
    _offset += _sent;
    _client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    if (_offset >= _fileSize + _headerSize) // TODO only between boundary is the filesize
    {
        cout << "completed get request for file: " << _client._filenamePath << ", on fd: " << _client._fd << endl; //test
        RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
        return true;
    }
    _fileBuffer = _fileBuffer.erase(0, _sent);
    return false;
}
