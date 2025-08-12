#include <HandleTransfer.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>
#include <sys/epoll.h>

// POST
HandlePostTransfer::HandlePostTransfer(Client &client, size_t bytesRead, string buffer)
: HandleTransfer(client, -1), _foundBoundary(false), _searchContentDisposition(false)
{
    _bytesReadTotal = bytesRead;
    _fileBuffer = buffer;
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
}

HandlePostTransfer &HandlePostTransfer::operator=(const HandlePostTransfer &other)
{
    if (this != &other) {
        // _client is a reference and cannot be assigned
        _fd = other._fd;
        _fileBuffer = other._fileBuffer;
        _fileNamePaths = other._fileNamePaths;
        _bytesReadTotal = other._bytesReadTotal;
        _foundBoundary = other._foundBoundary;
        _searchContentDisposition = other._searchContentDisposition;
    }
    return *this;
}

// return 0 if should continue reading, 1 if should stop reading and finished 2 if should rerun without reading
int HandlePostTransfer::validateFinalCRLF()
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
    if (foundReturn == 2 && strncmp(_fileBuffer.data(), "--\r\n", 4) == 0 && _fileBuffer.size() <= 4)
    {
        FileDescriptor::closeFD(_fd);
        _fd = -1;
        RunServers::logMessage(5, "POST success, clientFD: ", _client._fd, ", filenamePath: ", _client._filenamePath);
        size_t absolutePathSize = RunServers::getServerRootDir().size();
        string relativePath = "." + _client._filenamePath.substr(absolutePathSize) + '\n';
        string headers =  HttpRequest::HttpResponse(_client, 201, ".txt", relativePath.size()) + relativePath;
        std::cout << escapeSpecialChars(headers) << std::endl; //testcout
        send(_client._fd, headers.data(), headers.size(), 0); // TODO: check if send is successful and if needs its own handle???
        return true;
    }
    if (_fileBuffer.size() > 4)
        errorPostTransfer(_client, 400, "post request has more characters then allowed between boundary and return characters");
    return false;
}

size_t HandlePostTransfer::FindBoundaryAndWrite(ssize_t &bytesWritten)
{
    size_t boundaryBufferSize = _client._bodyBoundary.size() + 4; // \r\n--boundary
    size_t writeSize = (boundaryBufferSize >= _fileBuffer.size()) ? 0 : _fileBuffer.size() - boundaryBufferSize;
    size_t boundaryFound = _fileBuffer.find(_client._bodyBoundary);
    if (boundaryFound != string::npos)
    {
        if (boundaryFound >= 4 && strncmp(_fileBuffer.data() + boundaryFound - 4, "\r\n--", 4) == 0)
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

bool HandlePostTransfer::searchContentDisposition()
{
    size_t bodyEnd = _fileBuffer.find(CRLF2);
    if (bodyEnd == string::npos)
        return false;
    HttpRequest::getBodyInfo(_client, _fileBuffer);
    _fileBuffer = _fileBuffer.erase(0, bodyEnd + 4);
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
    _fileNamePaths.push_back(_client._filenamePath);
    _searchContentDisposition = false;
    return true;
}

bool HandlePostTransfer::handlePostTransfer(bool readData)
{
    try
    {
        if (readData == true)
        {
            char buff[RunServers::getClientBufferSize()];
            size_t bytesReceived = RunServers::receiveClientData(_client, buff);
            _bytesReadTotal += bytesReceived;
            _fileBuffer.append(buff, bytesReceived);
        }
        if (_client._isCgi)
        {
            return handlePostCgi();
        }
        while (true)
        {
            if (_bytesReadTotal > _client._contentLength)
                throw ErrorCodeClientException(_client, 400, "Content length smaller then body received for client with fd: " + to_string(_client._fd));
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
                _foundBoundary = true;
            }
            else
                return false;
        }

        return false;
    }
    catch(const exception& e)
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


void HandlePostTransfer::errorPostTransfer(Client &client, uint16_t errorCode, string errMsg)
{
    FileDescriptor::closeFD(_fd);
    for (const auto &filePath : _fileNamePaths)
    {
        if (remove(filePath.data()) != 0)
            cout << "remove failed on file: " << filePath << endl;
    }
    throw ErrorCodeClientException(client, errorCode, errMsg + strerror(errno) + ", on file with fileDescriptor: " + to_string(_fd));
}

void HandlePostTransfer::parseContentDisposition(Client &client, string_view &buffer)
{
    size_t bodyEnd = buffer.find(CRLF2);
    if (bodyEnd == string_view::npos)
        throw ErrorCodeClientException(client, 400, "Missing double CRLF after Content-Disposition");
    
    string buf = string(buffer.substr(0, bodyEnd));
    HttpRequest::getBodyInfo(client, buf);
    buffer.remove_prefix(bodyEnd + CRLF2_LEN);
}

bool HandlePostTransfer::validateBoundaryTerminator(Client &client, string_view &buffer, bool &needsContentDisposition)
{
    size_t crlfPos = buffer.find(CRLF);
    
    // Empty line - signals start of content disposition
    if (crlfPos == 0)
    {
        buffer.remove_prefix(CRLF_LEN);
        needsContentDisposition = true;
        return false;
    }
    
    // Check for boundary terminator "--\r\n"
    const string terminator = string("--") + CRLF;
    if (crlfPos == 2 && buffer.size() >= terminator.size() && 
        strncmp(buffer.data(), terminator.data(), terminator.size()) == 0)
        return true;
    throw ErrorCodeClientException(client, 400, "invalid post request to cgi)");
}

bool HandlePostTransfer::validateMultipartPostSyntax(Client &client, string &input)
{
    if (input.size() != client._contentLength)
        throw ErrorCodeClientException(client, 400, "Content-Length does not match body size");
    string_view buffer = string_view(input);
    bool foundBoundary = false;
    bool needsContentDisposition = false;
    
    while (!buffer.empty())
    {
        if (foundBoundary)
        {
            if (validateBoundaryTerminator(client, buffer, needsContentDisposition))
                return true; // Found end of multipart data
            foundBoundary = false;
            continue;
        }
        if (needsContentDisposition)
        {
            parseContentDisposition(client, buffer);
            needsContentDisposition = false;
            continue;
        }
        size_t boundaryPos = buffer.find(client._bodyBoundary);
        if (boundaryPos == string_view::npos)
            throw ErrorCodeClientException(client, 400, "Expected boundary not found in multipart data");
        buffer.remove_prefix(boundaryPos + client._bodyBoundary.size());
        foundBoundary = true;
    }
    throw ErrorCodeClientException(client, 400, "Incomplete multipart data - missing terminator");
}

bool HandlePostTransfer::handlePostCgi()
{
    if (_fileBuffer.find(string(_client._bodyBoundary) + "--" + CRLF) == string::npos)
    {
        // std::cout << "count" << std::endl; //testcout
        return false;
    }

    if (validateMultipartPostSyntax(_client, _fileBuffer) == true)
    {
        std::cout << "correct cgi syntax for post request" << std::endl; //testcout
        // send body to pipe for stdin of cgi
        // std::cout << _fileBuffer << std::endl; //testcout
        // std::cout << "_client._body.size() " << _client._body.size() << std::endl; //testcout
        // std::cout << "_fileBuffer.size() " << _fileBuffer.size() << std::endl; //testcout
        HttpRequest::handleCgi(_client, _fileBuffer);
        return true;
    }
    else
        throw ErrorCodeClientException(_client, 400, "Malformed POST request syntax for CGI");
    return false;
}
