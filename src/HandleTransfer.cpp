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

HandleTransfer::HandleTransfer(Client &client, int fd, size_t bytesRead, string boundary)
: _client(client), _fd(fd), _bytesReadTotal(bytesRead), _fileBuffer(boundary), _foundBoundary(false), _searchContentDisposition(false)
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
            throw RunServers::ClientException(string("handlingTransfer read: ") + strerror(errno) + ", fd: " + to_string(_fd) + ", on file: " + _client._rootPath);
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

void HandleTransfer::errorPostTransfer(Client &client, uint16_t errorCode, string errMsg, int fd)
{
    FileDescriptor::closeFD(fd);
    if (remove(client._rootPath.data()))
        std::cout << "remove failed on file:" << client._rootPath << std::endl;
    throw ErrorCodeClientException(client, errorCode, errMsg + strerror(errno) + ", on fileDescriptor: " + to_string(fd));
}

bool HandleTransfer::validateFinalCRLF()
{
    size_t foundReturn = _fileBuffer.find("\r\n");
    if (foundReturn == 0)
    {
        _fileBuffer.substr(2);
        _searchContentDisposition = true;
        // bool // needtofindNewcontentDisposition line true
        FileDescriptor::closeFD(_fd);
        _fd = -1;
        _foundBoundary = false;
        return false;
    }
    if (foundReturn != 2 && foundReturn != string::npos)
        errorPostTransfer(_client, 400, "post request has more characters then allowed between boundary and return characters", _fd);
    for (size_t i = 0; i < _fileBuffer.size(); ++i)
    {
        char c = _fileBuffer[i];
        if (c != '-' && c != '\r' && c != '\n')
        {
            errorPostTransfer(_client, 400, "post request has invalid characters after boundary", _fd);
        }
    }
    if (foundReturn == 2 && foundReturn + 2 == _fileBuffer.size())
    {
        if (_bytesReadTotal != _client._contentLength)
            throw ErrorCodeClientException(_client, 400, "post request has wrong content length: " + to_string(_bytesReadTotal) + ", expected: " + to_string(_client._contentLength));
        FileDescriptor::closeFD(_fd);
        string body = _client._rootPath + '\n';
        string headers =  HttpRequest::HttpResponse(_client, 200, ".txt", body.size()) + body;
        send(_client._fd, headers.data(), headers.size(), 0);
        return true;
    }
    if (_fileBuffer.size() > 4)
        errorPostTransfer(_client, 400, "post request has more characters then allowed between boundary and return characters", _fd);
    return false;
}

size_t HandleTransfer::FindBoundaryAndWrite(ssize_t &bytesWritten)
{
    size_t boundaryBufferSize = _client._bodyBoundary.size() + 4; // \r\n--boundary
    size_t writeSize = (boundaryBufferSize >= _fileBuffer.size()) ? 0 : _fileBuffer.size() - boundaryBufferSize;
    size_t boundaryFound = _fileBuffer.find(_client._bodyBoundary);
    if (boundaryFound != string::npos)
    {
        if (_fileBuffer.find("\r\n") != boundaryFound - 4)
            throw ErrorCodeClientException(_client, 400, "post request has more characters then allowed between content and boundary");
        writeSize = boundaryFound - 4;
    }
    if (writeSize > 0)
    {
        bytesWritten = write(_fd, _fileBuffer.data(), writeSize);
        if (bytesWritten == -1)
            ErrorCodeClientException(_client, 500, "write failed post request: " + string(strerror(errno)));
        _fileBuffer = _fileBuffer.substr(bytesWritten);
    }
    return boundaryFound;
}

// void HandleTransfer::searchContentDisposition()
// {
//     size_t bodyEnd = _fileBuffer.find("\r\n\r\n");
//     if (bodyEnd == string::npos)
//         return ;
//     HttpRequest::getBodyInfo(_client);
//     _client._rootPath = _client._rootPath.substr(0, _client._rootPath.find_last_of('/'));
//     _client._rootPath = _client._rootPath + "/" + string(_client._filename); // here to append filename for post
//     std::cout << "rootpath after: " << _client._rootPath << std::endl; //testcout
//     _fileBuffer = _fileBuffer.substr(bodyEnd + 4);
//     _fd = open(_client._rootPath.data(), O_WRONLY | O_TRUNC | O_CREAT, 0700);
//     bool hasBoundaryPrefix = false;
//     if (_fd == -1)
//     {
//         if (errno == EACCES)
//             throw ErrorCodeClientException(_client, 403, "access not permitted for post on file: " + _client._rootPath);
//         else
//             throw ErrorCodeClientException(_client, 500, "couldn't open file because: " + string(strerror(errno)) + ", on file: " + _client._rootPath);
//     }
//     _searchContentDisposition = false;
// }

bool HandleTransfer::handlePostTransfer()
{

    try
    {
        char buff[CLIENT_BUFFER_SIZE];
        size_t bytesReceived = RunServers::receiveClientData(_client, buff);
        _bytesReadTotal += bytesReceived;
        // if (_searchContentDisposition == true)
        // {
        //     searchContentDisposition();
        // }
        if (_bytesReadTotal > _client._contentLength)
            throw ErrorCodeClientException(_client, 413, "Content length smaller then body received for fd: " + to_string(_fd));
        _fileBuffer.append(buff, bytesReceived);
        if (_foundBoundary == true)
            return validateFinalCRLF();
        ssize_t bytesWritten = 0;
        size_t boundaryFound = FindBoundaryAndWrite(bytesWritten);
        if (boundaryFound != string::npos)
        {
            size_t offset = 0;
            _fileBuffer = _fileBuffer.substr(_client._bodyBoundary.size() + boundaryFound - bytesWritten);
            RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
            RunServers::logMessage(5, "POST success, clientFD: ", _client._fd, ", rootpath: ", _client._rootPath);
            _foundBoundary = true;
            return (validateFinalCRLF());
        }
        return false;
    }
    catch(const std::exception& e)
    {
        cerr << "Error in handlePostTransfer: " << e.what() << endl;
        FileDescriptor::closeFD(_fd);
        if (remove(_client._rootPath.data()))
            cerr << "remove failed on file:" << _client._rootPath << "with error: " << strerror(errno) << std::endl;
        RunServers::cleanupClient(_client);
    }
    catch (const ErrorCodeClientException &e)
    {
        errorPostTransfer(_client, e.getErrorCode(), e.getMessage(), _fd);
    }
    return true;
}
