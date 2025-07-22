#include <HandleTransfer.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>
#include <Client.hpp>

#include <sys/epoll.h>

HandleTransfer::HandleTransfer(Client &client, int fd, string &responseHeader, size_t fileSize)
: _client(client), _fd(fd), _fileBuffer(responseHeader), _fileSize(fileSize)
{
    _headerSize = responseHeader.size();
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLOUT);
}

HandleTransfer::HandleTransfer(Client &client, size_t bytesRead, string buffer)
: _client(client), _fd(-1), _fileBuffer(buffer), _bytesReadTotal(bytesRead), _foundBoundary(false), _searchContentDisposition(false)
{
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
}

HandleTransfer::HandleTransfer(Client &client)
: _client(client), _isChunked(true)
{
    // _body = body;
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
    _client.setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    if (_offset >= _fileSize + _headerSize) // TODO only between boundary is the filesize
    {
        std::cout << "completed get request for file: " << _client._filenamePath << ", on fd: " << _client._fd << std::endl; //test
        RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
        _epollout_enabled = false;
        return true;
    }
    _fileBuffer = _fileBuffer.erase(0, _sent);
    return false;
}

void HandleTransfer::errorPostTransfer(Client &client, uint16_t errorCode, string errMsg)
{
    FileDescriptor::closeFD(_fd);
    for (const auto &filePath : _fileNamePaths)
    {
        if (remove(filePath.data()) != 0)
            std::cout << "remove failed on file: " << filePath << std::endl;
    }
    throw ErrorCodeClientException(client, errorCode, errMsg + strerror(errno) + ", on file with fileDescriptor: " + to_string(_fd));
}

// return 0 if should continue reading, 1 if should stop reading and finished 2 if should rerun without reading
int HandleTransfer::validateFinalCRLF()
{
    size_t foundReturn = _fileBuffer.find(CRLF);
    if (foundReturn == 0)
    {
        _fileBuffer = _fileBuffer.erase(0, 2);
        _searchContentDisposition = true;
        FileDescriptor::closeFD(_fd);
        _fd = -1;
        _foundBoundary = false;
        return 2;
    }
    if (foundReturn != 2 && foundReturn != string::npos)
        errorPostTransfer(_client, 400, "post request has more characters then allowed between boundary and return characters");
    for (size_t i = 0; i < _fileBuffer.size(); ++i)
    {
        char c = _fileBuffer[i];
        if (c != '-' && c != '\r' && c != '\n')
        {
            errorPostTransfer(_client, 400, "post request has invalid characters after boundary");
        }
    }
    if (foundReturn == 2 && foundReturn + 2 == _fileBuffer.size())
    {
        if (_bytesReadTotal != _client._contentLength)
            throw ErrorCodeClientException(_client, 400, "post request has wrong content length: " + to_string(_bytesReadTotal) + ", expected: " + to_string(_client._contentLength));
        FileDescriptor::closeFD(_fd);
        _fd = -1;
        string body = _client._filenamePath + '\n';
        string headers =  HttpRequest::HttpResponse(_client, 201, ".txt", body.size()) + body;
        send(_client._fd, headers.data(), headers.size(), 0);
        return true;
    }
    if (_fileBuffer.size() > 4)
        errorPostTransfer(_client, 400, "post request has more characters then allowed between boundary and return characters");
    return false;
}

size_t HandleTransfer::FindBoundaryAndWrite(ssize_t &bytesWritten)
{
    size_t boundaryBufferSize = _client._bodyBoundary.size() + 4; // \r\n--boundary
    size_t writeSize = (boundaryBufferSize >= _fileBuffer.size()) ? 0 : _fileBuffer.size() - boundaryBufferSize;
    size_t boundaryFound = _fileBuffer.find(_client._bodyBoundary);
    if (boundaryFound != string::npos)
    {
        if (strncmp(_fileBuffer.data() + boundaryFound - 4, "\r\n--", 4) == 0)
            writeSize = boundaryFound - 4;
        else if (strncmp(_fileBuffer.data() + boundaryFound - 2, "--",2 ) == 0)
            writeSize = boundaryFound - 2;
        else
            throw ErrorCodeClientException(_client, 400, "post request has more characters then allowed between content and boundary");
    }
    if (writeSize > 0)
    {
        bytesWritten = write(_fd, _fileBuffer.data(), writeSize);
        if (bytesWritten == -1)
            ErrorCodeClientException(_client, 500, "write failed post request: " + string(strerror(errno)));
        _fileBuffer = _fileBuffer.erase(0, bytesWritten);
    }
    return boundaryFound;
}

bool HandleTransfer::searchContentDisposition()
{
    size_t bodyEnd = _fileBuffer.find(CRLF2);
    if (bodyEnd == string::npos)
        return false;
    _client._body = _fileBuffer.substr(0, bodyEnd);
    HttpRequest::getBodyInfo(_client);
    _fileBuffer = _fileBuffer.erase(0, bodyEnd + CRLF2_LEN);
    _client._filenamePath = _client._rootPath + "/" + string(_client._filename); // here to append filename for post
    _fd = open(_client._filenamePath.data(), O_WRONLY | O_TRUNC | O_CREAT, 0700);
    if (_fd == -1)
    {
        if (errno == EACCES)
            throw ErrorCodeClientException(_client, 403, "access not permitted for post on file: " + _client._filenamePath);
        else
            throw ErrorCodeClientException(_client, 500, "couldn't open file because: " + string(strerror(errno)) + ", on file: " + _client._filenamePath);
    }
    _fileNamePaths.push_back(_client._filenamePath);
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
        while (true)
        {
            if (_bytesReadTotal > _client._contentLength)
                throw ErrorCodeClientException(_client, 413, "Content length smaller then body received for client with fd: " + to_string(_client._fd));
            if (_searchContentDisposition == true && searchContentDisposition() == false)
                return false;
            if (_foundBoundary == true)
            {
                int result = validateFinalCRLF();
                if (result == 2)
                    continue ;
                return (result == 1); // if result is true, then we are done with the post transfer
            }
            ssize_t bytesWritten = 0;
            size_t boundaryFound = FindBoundaryAndWrite(bytesWritten);
            if (boundaryFound != string::npos)
            {
                _fileBuffer = _fileBuffer.erase(0, _client._bodyBoundary.size() + boundaryFound - bytesWritten);
                RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
                RunServers::logMessage(5, "POST success, clientFD: ", _client._fd, ", filenamePath: ", _client._filenamePath);
                _foundBoundary = true;
            }
            else
                return false;
        }

        return false;
    }
    catch(const std::exception& e)
    {
        _client._keepAlive = false;
        errorPostTransfer(_client, 500, "Error in handlePostTransfer: " + string(e.what()));
    }
    catch (const ErrorCodeClientException &e)
    {
        errorPostTransfer(_client, e.getErrorCode(), e.getMessage());
    }
    return true;
}

bool HandleTransfer::extractChunkSize()
{
    size_t crlfPos = _client._body.find(CRLF, _bodyPos);

    if (crlfPos != string::npos)
    {

        string chunkSizeLine = _client._body.substr(_bodyPos, crlfPos);
        validateChunkSizeLine(chunkSizeLine);

        _chunkTargetSize = parseChunkSize(chunkSizeLine);
        _chunkDataStart = crlfPos + CRLF_LEN;
        return true;
    }
    return false;
}

bool    HandleTransfer::decodeChunk()
{
    if (extractChunkSize() == false)
    {
        return false;
    }
    string &body = _client._body;

    size_t chunkDataEnd = _chunkDataStart + _chunkTargetSize;

    if (body.size() >= chunkDataEnd + CRLF_LEN &&
        body[chunkDataEnd] == '\r' &&
        body[chunkDataEnd + 1] == '\n')
    {
        // _unchunkedBody.append(body.data() + _chunkDataStart, chunkDataEnd - _chunkDataStart);
        _bodyPos = chunkDataEnd + CRLF_LEN;
        _fileBuffer.append(body.data() + _chunkDataStart, chunkDataEnd - _chunkDataStart);
    }
    else
    {
        ErrorCodeClientException(_client, 400, "Chunk data missing CRLF");
    }
    return true;
}

void HandleTransfer::appendToBody()
{
    char   buff[RunServers::getClientBufferSize()];
    size_t bytesReceived = RunServers::receiveClientData(_client, buff);
    _client._body.append(buff, bytesReceived);
}

bool HandleTransfer::FinalCrlfCheck()
{
    size_t crlfPos = _unchunkedBody.find("--" + string(_client._bodyBoundary) + "--\r\n");

    if (crlfPos == string::npos)
    {
        return false;
    }

    return true;
}

bool HandleTransfer::handleChunkTransfer()
{

    decodeChunk();
    handlePostTransfer(false);
    return false;
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