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
    _epollout_enabled = false;
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

bool RunServers::handleGetTransfer(HandleTransfer &ht)
{
    char buff[CLIENT_BUFFER_SIZE];
    if (ht._fd != -1)
    {
        ssize_t bytesRead = read(ht._fd, buff, CLIENT_BUFFER_SIZE);
        if (bytesRead == -1)
            throw ClientException(string("handlingTransfer read: ") + strerror(errno));
        size_t _bytesRead = static_cast<size_t>(bytesRead);
        ht._bytesReadTotal += _bytesRead;
        if (_bytesRead > 0)
        {
            ht._fileBuffer.append(buff, _bytesRead);
            
            if (ht._epollout_enabled == false)
            {
                setEpollEvents(ht._client._fd, EPOLL_CTL_MOD, EPOLLOUT);
                ht._epollout_enabled = true;
            }
        }
        if (_bytesRead == 0 || ht._bytesReadTotal >= ht._fileSize)
        {
            // EOF reached, close file descriptor if needed
            FileDescriptor::closeFD(ht._fd);
            ht._fd = -1;
        }
    }
    ssize_t sent = send(ht._client._fd, ht._fileBuffer.c_str(), ht._fileBuffer.size(), 0);
    if (sent == -1)
        throw ClientException(string("handlingTransfer send: ") + strerror(errno));
    size_t _sent = static_cast<size_t>(sent);
    ht._offset += _sent;
    if (ht._offset >= ht._fileSize + ht._headerSize) // TODO only between boundary is the filesize
    {
        setEpollEvents(ht._client._fd, EPOLL_CTL_MOD, EPOLLIN);
        ht._epollout_enabled = false;
        return true;
    }
    ht._fileBuffer = ht._fileBuffer.substr(_sent);
    return false;
}

bool RunServers::handlePostTransfer(HandleTransfer &ht)
{
    try
    {
        char buff[CLIENT_BUFFER_SIZE];
        size_t bytesReceived = receiveClientData(ht._client, buff);
        size_t byteswrite = bytesReceived;

        if (bytesReceived > ht._fileSize - ht._bytesWrittenTotal)
            byteswrite = ht._fileSize - ht._bytesWrittenTotal;
        ssize_t bytesWritten = write(ht._fd, buff, byteswrite);
        if (bytesWritten == -1)
        {
            // remove and filedesciptor
            // if (!handle._filename.empty()) // assuming HandleTransfer has a _filename member
            // 	remove(handle._filename.data());
            FileDescriptor::closeFD(ht._fd);
            throw ErrorCodeClientException(ht._client, 500, string("write failed HandlePostTransfer: ") + strerror(errno));
        }
        ht._bytesWrittenTotal += static_cast<size_t>(bytesWritten);

        if (ht._bytesWrittenTotal == ht._fileSize)
        {
            ht._fileBuffer.append(buff + bytesWritten, bytesReceived - byteswrite);
            if (ht._fileBuffer.find("--" + string(ht._client._bodyBoundary) + "--\r\n") == 2)
            {
                FileDescriptor::closeFD(ht._fd);
                string ok = HttpRequest::HttpResponse(200, "", 0);
                send(ht._client._fd, ok.data(), ok.size(), 0);
                return true;
            }
        }
        return false;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return true;
}