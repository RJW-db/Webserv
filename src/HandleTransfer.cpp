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

HandleTransfer::HandleTransfer(Client &client, int fd, size_t bytesRead, string boundary, bool hasBoundaryPrefix)
: _client(client), _fd(fd), _bytesReadTotal(bytesRead), _fileBuffer(boundary), _foundEndingBoundary(false), _hasBoundaryPrefix(hasBoundaryPrefix)
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

void HandleTransfer::readToBuf()
{
    if (_fd != -1)
    {
        char buff[CLIENT_BUFFER_SIZE];
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
            FileDescriptor::closeFD(_fd);
            _fd = -1;
        }
    }
}

bool HandleTransfer::handleGetTransfer()
{
    readToBuf();
    ssize_t sent = send(_client._fd, _fileBuffer.c_str(), _fileBuffer.size(), 0);
    if (sent == -1)
        throw RunServers::ClientException(string("handlingTransfer send: ") + strerror(errno)); // TODO throw out client and remove handleTransfer
    size_t _sent = static_cast<size_t>(sent);
    _offset += _sent;
    _client.setDisconnectTime(disconnectDelaySeconds);
    if (_offset >= _fileSize + _headerSize) // TODO only between boundary is the filesize
    {
        std::cout << "completed get request for file: " << _client._rootPath << ", on fd: " << _client._fd << std::endl; //test
        RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
        _epollout_enabled = false;
        return true;
    }
    _fileBuffer = _fileBuffer.substr(_sent);
    return false;
}

bool HandleTransfer::foundBoundaryPost(Client &client, string &boundaryBuffer, int fd)
{
    if (boundaryBuffer.find("--" + string(client._bodyBoundary) + "--") == 2)
    {
        FileDescriptor::closeFD(fd);
        string body = client._rootPath + '\n';
        string headers =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: " + string(client._keepAlive ? "keep-alive" : "close") + "\r\n"
            "Content-Length: " +
            to_string(body.size()) + "\r\n"
                                     "\r\n" +
            body;

        send(client._fd, headers.data(), headers.size(), 0);
        RunServers::setEpollEvents(client._fd, EPOLL_CTL_MOD, EPOLLIN);
        RunServers::logMessage(5, "POST success, clientFD: ", client._fd, ", rootpath: ", client._rootPath);
        return true;
    }

    return false;
}

void HandleTransfer::errorPostTransfer(Client &client, uint16_t errorCode, string errMsg, int fd)
{
    if (remove(client._rootPath.data()))
        std::cout << "remove failed on file:" << client._rootPath << std::endl;
    FileDescriptor::closeFD(fd);
    throw ErrorCodeClientException(client, errorCode, errMsg + strerror(errno) + ", on fileDescriptor: " + to_string(fd));
}

// bool HandleTransfer::handlePostTransfer()
// {
//     try
//     {
//         char buff[CLIENT_BUFFER_SIZE];
//         size_t bytesReceived = RunServers::receiveClientData(_client, buff);
//         size_t byteswrite = bytesReceived;

//         if (bytesReceived > _fileSize - _bytesWrittenTotal)
//             byteswrite = _fileSize - _bytesWrittenTotal;
//         ssize_t bytesWritten = write(_fd, buff, byteswrite);
//         _client.setDisconnectTime(disconnectDelaySeconds);
//         if (bytesWritten == -1)
//             errorPostTransfer(_client, 500, "write failed post request: " + string(strerror(errno)), _fd);
//         _bytesWrittenTotal += static_cast<size_t>(bytesWritten);
//         if (_bytesWrittenTotal == _fileSize)
//         {
//             _fileBuffer.append(buff + bytesWritten, bytesReceived - byteswrite);
//             return (foundBoundaryPost(_client, _fileBuffer, _fd));
//         }
//         return false;
//     }
//     catch(const exception& e)
//     {
//         cerr << e.what() << '\n';
//         return true;
//     }
//     return true;
// }

bool HandleTransfer::handlePostTransfer()
{
    try
    {
        char buff[CLIENT_BUFFER_SIZE];
        size_t bytesReceived = RunServers::receiveClientData(_client, buff);
        _bytesReadTotal += bytesReceived;
        // size_t byteswrite = bytesReceived;
        size_t boundaryBufferSize = _client._bodyBoundary.size() + 2 + (_hasBoundaryPrefix * 2); // \r\n\r\n--boundary
        _fileBuffer.append(buff, bytesReceived);
        if (_foundEndingBoundary == true)
        {
            size_t foundReturn = _fileBuffer.find("\r\n");
            
            if (foundReturn != _hasBoundaryPrefix * 2 && foundReturn != string::npos)
                throw ErrorCodeClientException(_client, 400, "post request has more characters then allowed between boundary and return characters");
            if (foundReturn == _hasBoundaryPrefix * 2 && foundReturn + 2 == _fileBuffer.size() )
            {
                std::cout << "returning in foundendingBoundary" << std::endl; //testcout
                return true;
            }

            if (_fileBuffer.size() > 2 + _hasBoundaryPrefix * 2)
                throw ErrorCodeClientException(_client, 400, "post request has more characters then allowed between boundary and return characters");
            return false;
        }
        size_t writeSize = (boundaryBufferSize >= _fileBuffer.size()) ? 0 : _fileBuffer.size() - boundaryBufferSize;
        ssize_t boundaryFound = _fileBuffer.find(_client._bodyBoundary);
        if (boundaryFound != string::npos)
        {
            for (ssize_t i = boundaryFound; i >= 0; i--)
            {
                if (_fileBuffer[i] == '\n')
                {
                    if (i > 0 && _fileBuffer[i - 1] == '\r')
                    {
                        if (i > 1 && _fileBuffer[i - 1] != '\n')
                        {
                            writeSize = i - 1;
                            break;
                        }
                    }
                }
            }
        }
        ssize_t bytesWritten = 0;
        if (writeSize > 0)
        {
            bytesWritten = write(_fd, _fileBuffer.data(), writeSize);
            if (bytesWritten == -1)
                HandleTransfer::errorPostTransfer(_client, 500, "write failed post request: " + string(strerror(errno)), _fd);
            if (boundaryFound != string::npos)
            _fileBuffer = _fileBuffer.substr(bytesWritten);
        }
        if (boundaryFound != string::npos)
        {
            FileDescriptor::closeFD(_fd);
            size_t offset = 0;
            if (_hasBoundaryPrefix == true)
                offset = 2;
            if (boundaryFound - bytesWritten != offset + 2)
                throw ErrorCodeClientException(_client, 400, "post request has more characters then allowed between content and boundary");
            _fileBuffer = _fileBuffer.substr(_client._bodyBoundary.size() + boundaryFound - bytesWritten);
            string body = _client._rootPath + '\n';
            string headers =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: " +
                string(_client._keepAlive ? "keep-alive" : "close") + "\r\n"
                "Content-Length: " +
                to_string(body.size()) + "\r\n"
                                         "\r\n" +
                body;
            send(_client._fd, headers.data(), headers.size(), 0);
            RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
            RunServers::logMessage(5, "POST success, clientFD: ", _client._fd, ", rootpath: ", _client._rootPath);
            _foundEndingBoundary = true;
            std::cout << "here" << std::endl; //testcout
            size_t foundReturn = _fileBuffer.find("\r\n");
            if (foundReturn != offset && foundReturn != string::npos)
                throw ErrorCodeClientException(_client, 400, "after post boundary and \\r\\n found more characters");
            if (foundReturn != string::npos)
            {
                std::cout << "returning here" << std::endl; //testcout
                return true;
            }
            return false;
        }
        return false;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return true;
    }
}
