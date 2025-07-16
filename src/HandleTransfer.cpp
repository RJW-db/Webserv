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
    _headerSize = responseHeader.size();
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLOUT);
}

HandleTransfer::HandleTransfer(Client &client, int fd, size_t bytesRead, string buffer)
: _client(client), _fd(fd), _bytesReadTotal(bytesRead), _fileBuffer(buffer)
{
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
}

HandleTransfer::HandleTransfer(Client &client, string body)
: _client(client), _isChunked(true)
{
    _body = body;
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
        char buff[RunServers::getClientBufferSize()];
        ssize_t bytesRead = read(_fd, buff, RunServers::getClientBufferSize());
        if (bytesRead == -1)
            throw RunServers::ClientException(string("handlingTransfer read: ") + strerror(errno) + ", fd: " + to_string(_fd) + ", on file: " + _client._filenamePath);
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
    ssize_t sent = send(_client._fd, _fileBuffer.c_str(), _fileBuffer.size(), MSG_NOSIGNAL);
    if (sent == -1)
        throw RunServers::ClientException(string("handlingTransfer send: ") + strerror(errno)); // TODO throw out client and remove handleTransfer
    size_t _sent = static_cast<size_t>(sent);
    _offset += _sent;
    _client.setDisconnectTime(disconnectDelaySeconds);
    if (_offset >= _fileSize + _headerSize) // TODO only between boundary is the filesize
    {
        std::cout << "completed get request for file: " << _client._filenamePath << ", on fd: " << _client._fd << std::endl; //test
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
    if (remove(client._filenamePath.data()))
        std::cout << "remove failed on file: " << client._filenamePath << std::endl;
    throw ErrorCodeClientException(client, errorCode, errMsg + strerror(errno) + ", on fileDescriptor: " + to_string(fd));
}



// return 0 if should continue reading, 1 if should stop reading 2 if should continue function
bool HandleTransfer::validateFinalCRLF()
{
    size_t foundReturn = _fileBuffer.find("\r\n");
    if (foundReturn == 0)
    {
        _fileBuffer = _fileBuffer.substr(2);
        _searchContentDisposition = true;
        FileDescriptor::closeFD(_fd);
        _fd = -1;
        _foundBoundary = false;
        return (handlePostTransfer(false));
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
        _fd = -1;
        string body = _client._filenamePath + '\n';
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

bool HandleTransfer::searchContentDisposition()
{
    size_t bodyEnd = _fileBuffer.find("\r\n\r\n");
    if (bodyEnd == string::npos)
        return false;
    _client._body = _fileBuffer.substr(0, bodyEnd);
    HttpRequest::getBodyInfo(_client);
    _fileBuffer = _fileBuffer.substr(bodyEnd + 4);
    _client._filenamePath = _client._rootPath + "/" + string(_client._filename); // here to append filename for post
    _fd = open(_client._filenamePath.data(), O_WRONLY | O_TRUNC | O_CREAT, 0700);
    bool hasBoundaryPrefix = false;
    if (_fd == -1)
    {
        if (errno == EACCES)
            throw ErrorCodeClientException(_client, 403, "access not permitted for post on file: " + _client._filenamePath);
        else
            throw ErrorCodeClientException(_client, 500, "couldn't open file because: " + string(strerror(errno)) + ", on file: " + _client._filenamePath);
    }
    FileDescriptor::setFD(_fd);
    _searchContentDisposition = false;
    return true;
}

bool HandleTransfer::handlePostTransfer(bool readData)
{
    try
    {
        char buff[RunServers::getClientBufferSize()];
        if (readData == true)
        {
            size_t bytesReceived = RunServers::receiveClientData(_client, buff);
            _bytesReadTotal += bytesReceived;
            _fileBuffer.append(buff, bytesReceived);
        }
        if (_bytesReadTotal > _client._contentLength)
            throw ErrorCodeClientException(_client, 413, "Content length smaller then body received for fd: " + to_string(_fd));
        if (_searchContentDisposition == true && searchContentDisposition() == false)
            return false;
        if (_foundBoundary == true)
            return (validateFinalCRLF());
        ssize_t bytesWritten = 0;
        size_t boundaryFound = FindBoundaryAndWrite(bytesWritten);
        if (boundaryFound != string::npos)
        {
            size_t offset = 0;
            _fileBuffer = _fileBuffer.substr(_client._bodyBoundary.size() + boundaryFound - bytesWritten);
            RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
            RunServers::logMessage(5, "POST success, clientFD: ", _client._fd, ", filenamePath: ", _client._filenamePath);
            _foundBoundary = true;
            return (validateFinalCRLF());
        }
        return false;
    }
    catch(const std::exception& e)
    {
        cerr << "Error in handlePostTransfer: " << e.what() << endl;
        FileDescriptor::closeFD(_fd);
        if (remove(_client._filenamePath.data()))
            cerr << "remove failed on file:" << _client._filenamePath << ", with error: " << strerror(errno) << std::endl;
        RunServers::cleanupClient(_client);
    }
    catch (const ErrorCodeClientException &e)
    {
        errorPostTransfer(_client, e.getErrorCode(), e.getMessage(), _fd);
    }
    return true;
}

bool HandleTransfer::extractChunkSize()
{
    size_t crlfPos = _body.find(CRLF, _bodyPos);
    if (crlfPos != string::npos)
    {
        string chunkSizeLine = _body.substr(_bodyPos, crlfPos);
        validateChunkSizeLine(chunkSizeLine);
        
        _chunkTargetSize = parseChunkSize(chunkSizeLine);
        _chunkBodyPos = crlfPos + CRLF_LEN;
        return true;
    }
    return false;
}

bool HandleTransfer::processChunkBodyHeader()
{
    if (extractChunkSize() == false)
        return false;

    size_t chunkDataStart = _bodyPos + _chunkBodyPos;
    size_t chunkDataEnd = chunkDataStart + _chunkTargetSize;

    if (_body.size() >= chunkDataEnd + CRLF_LEN &&
        _body[chunkDataEnd] == '\r' &&
        _body[chunkDataEnd + 1] == '\n')
    {
        size_t boundaryStart = chunkDataStart + CRLF_LEN;
        string_view boundaryCheck(&_body[boundaryStart], _client._bodyBoundary.size());
        if (_body.substr(chunkDataStart, 2) != "--" ||
            boundaryCheck != _client._bodyBoundary)
        {
            ErrorCodeClientException(_client, 400, "Malformed boundary");
        }

        size_t headerStart = chunkDataStart + _client._bodyBoundary.size() + CRLF_LEN;
        size_t endBodyHeader = _body.find(CRLF2, headerStart);
        if (endBodyHeader == string::npos)
        {
            ErrorCodeClientException(_client, 400, "body header didn't end in \\r\\n\\r\\n");
        }
        _foundBoundary = true;
        _bodyHeader = _body.substr(chunkDataStart, endBodyHeader + CRLF2_LEN - _chunkBodyPos);
        HttpRequest::processHttpChunkBody(_client, _fd);

        size_t chunkEndCrlfPos = _bodyPos + endBodyHeader + CRLF2_LEN;
        _unchunkedBody = _body.substr(chunkEndCrlfPos/*  + CRLF_LEN */);

        // exit(0);
        if (_body.size() - _bodyPos < _chunkTargetSize + 2 ||
            _body[chunkDataStart + _chunkTargetSize] != '\r' ||
            _body[chunkDataStart + _chunkTargetSize + 1] != '\n')
        {
            throw ErrorCodeClientException(_client, 400, "Chunk didn't end in \\r\\n");
        }
        _bodyPos = chunkDataStart + _chunkTargetSize  + CRLF_LEN;
        // std::cout << escape_special_chars(_bodyHeader)<< std::endl; //testcout
        // std::cout << escape_special_chars(_unchunkedBody)<< std::endl; //testcout
        // exit(0);
        // std::cout << escape_special_chars(_body) << std::endl; //testcout
        if (_bodyPos == _body.size())
            std::cout << "ass big" << std::endl; //testcout

        // std::cout << _bodyPos << std::endl; //testcout
        // std::cout << _body.size() << std::endl; //testcout
        // std::cout << chunkDataEnd + CRLF_LEN << std::endl; //testcout
        // std::cout << escape_special_chars(&_body[_bodyPos]) << std::endl; //testcout
        exit(0);
        return true;
    }
    return false;
}

void    tmp()
{
    std::cout << "nothingness" << std::endl; //testcout
    exit(0);
}

void HandleTransfer::appendToBody()
{
    char   buff[RunServers::getClientBufferSize()];
    size_t bytesReceived = RunServers::receiveClientData(_client, buff);
    _body.append(buff, bytesReceived);
    _client.setDisconnectTime(disconnectDelaySeconds);
}


bool HandleTransfer::handleChunkTransfer()
{
    std::cout << "we got here" << std::endl; //testcout

    // std::cout << escape_special_chars(_body) << std::endl; //testcout
    if (_foundBoundary == false)
    {
        return processChunkBodyHeader();
    }
    else
    {
        tmp();
    }
    return true;
}

void HandleTransfer::validateChunkSizeLine(const string &input)
{
    if (input.size() < 1)
    {
        // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
    }

    // size_t hexPart = input.size() - 2;
    if (all_of(input.begin(), input.end(), ::isxdigit) == false)
    {
        // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
    }
        
    // if (input[hexPart] != '\r' && input[hexPart + 1] != '\n')
    // {
    //     // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
    // }
}

uint64_t HandleTransfer::parseChunkSize(const string &input)
{
    uint64_t chunkTargetSize;
    stringstream ss;
    ss << hex << input;
    ss >> chunkTargetSize;
    // cout << "chunkTargetSize " << chunkTargetSize << endl;
    // if (static_cast<size_t>(chunkTargetSize) > client._location.getClientBodySize())
    // {
    //     throw ErrorCodeClientException(client, 413, "Content-Length exceeds maximum allowed: " + to_string(chunkTargetSize)); // (413, "Payload Too Large");
    // }
    return chunkTargetSize;
}

void HandleTransfer::ParseChunkStr(const string &input, uint64_t chunkTargetSize)
{
    if (input.size() - 2 != chunkTargetSize)
    {
        // throw ErrorCodeClientException(client, 400, "Chunk data does not match declared chunk size");
    }

    if (input[chunkTargetSize] != '\r' || input[chunkTargetSize + 1] != '\n')
    {
        // throw ErrorCodeClientException(client, 400, "chunk data missing CRLF");
    }
}