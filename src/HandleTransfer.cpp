#include <HandleTransfer.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>

#include <sys/epoll.h>
// HandleTransfer::HandleTransfer(int clientFD, string &responseHeader, int fd)
// : _clientFD(clientFD), _header(responseHeader), _fd(fd)
HandleTransfer::HandleTransfer(Client &client, int fd, string &responseHeader, size_t fileSize)
: _client(client), _fd(fd), _fileBuffer(responseHeader), _fileSize(fileSize)
{
    _offset = 0;
    _bytesReadTotal = 0;
    _epollout_enabled = true;
    _headerSize = responseHeader.size();
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLOUT);
}

HandleTransfer::HandleTransfer(Client &client, int fd, size_t bytesWritten, size_t finalFileSize, string boundary)
: _client(client), _fd(fd), _bytesWrittenTotal(bytesWritten), _fileSize(finalFileSize), _fileBuffer(boundary)
{
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
}

HandleTransfer &HandleTransfer::operator=(const HandleTransfer& other)
{
    if (this != &other) {
        // _client is a reference and cannot be assigned
        _fd = other._fd;
        _fileBuffer = other._fileBuffer;
        _fileSize = other._fileSize;
        _offset = other._offset;
        _bytesReadTotal = other._bytesReadTotal;
        _headerSize = other._headerSize;
        _epollout_enabled = other._epollout_enabled;
		_bytesWrittenTotal = other._bytesWrittenTotal;
    }
    return *this;
}

bool HandleTransfer::handleGetTransfer()
{
    char buff[CLIENT_BUFFER_SIZE];
    if (_fd != -1)
    {
        ssize_t bytesRead = read(_fd, buff, CLIENT_BUFFER_SIZE);
        if (bytesRead == -1)
            throw RunServers::ClientException(string("handlingTransfer read: ") + strerror(errno) + ", fd: " + to_string(_fd));
        size_t _bytesRead = static_cast<size_t>(bytesRead);
        _bytesReadTotal += _bytesRead;
        if (_bytesRead > 0)
        {
            _fileBuffer.append(buff, _bytesRead);
            
            if (_epollout_enabled == false)
            {
                RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLOUT);
                _epollout_enabled = true;
            }
        }
        if (_bytesRead == 0 || _bytesReadTotal >= _fileSize)
        {
            // EOF reached, close file descriptor if needed
            FileDescriptor::closeFD(_fd);
            _fd = -1;
        }
    }
    ssize_t sent = send(_client._fd, _fileBuffer.c_str(), _fileBuffer.size(), 0);
    if (sent == -1)
        throw RunServers::ClientException(string("handlingTransfer send: ") + strerror(errno));
    size_t _sent = static_cast<size_t>(sent);
    _offset += _sent;
    if (_offset >= _fileSize + _headerSize) // TODO only between boundary is the filesize
    {
        // std::cout << "setting fd" << _client._fd << ", to epollin" << std::endl;
        RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
        _epollout_enabled = false;
        
        return true;
    }
    _fileBuffer = _fileBuffer.substr(_sent);
    return false;
}

bool HandleTransfer::handlePostTransfer()
{
    try
    {
        char buff[CLIENT_BUFFER_SIZE];
        size_t bytesReceived = RunServers::receiveClientData(_client, buff);
        size_t byteswrite = bytesReceived;

        if (bytesReceived > _fileSize - _bytesWrittenTotal)
            byteswrite = _fileSize - _bytesWrittenTotal;
        ssize_t bytesWritten = write(_fd, buff, byteswrite);
        if (bytesWritten == -1)
        {
            // remove and filedesciptor
            // if (!handle._filename.empty()) // assuming HandleTransfer has a _filename member
            // 	remove(handle._filename.data());
            FileDescriptor::closeFD(_fd);
            throw ErrorCodeClientException(_client, 500, string("write failed HandlePostTransfer: ") + strerror(errno));
        }
        _bytesWrittenTotal += static_cast<size_t>(bytesWritten);
        if (_bytesWrittenTotal == _fileSize)
        {
            _fileBuffer.append(buff + bytesWritten, bytesReceived - byteswrite);
            if (_fileBuffer.find("--" + string(_client._bodyBoundary) + "--\r\n") == 2)
            {
                FileDescriptor::closeFD(_fd);
                string body = _client._rootPath + '\n';
                string headers =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: " + to_string(body.size()) + "\r\n"
                    "\r\n" +
                    body;

                send(_client._fd, headers.data(), headers.size(), 0);
                return true;
            }
        }
        return false;
    }
    catch(const exception& e)
    {
        cerr << e.what() << '\n';
        return true;
    }
    return true;
}