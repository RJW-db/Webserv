#include <HandleTransfer.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>
#include <sys/epoll.h>
#include "Logger.hpp"


namespace {
    constexpr size_t BOUNDARY_PADDING = 4;  // for \r\n-- prefix
    constexpr size_t TERMINATOR_SIZE = 4;    // for --\r\n
    constexpr int FILE_PERMISSIONS = 0700;
    constexpr uint16_t HTTP_CREATED = 201;
}

/**
 * Constructor for HandlePostTransfer
 * Initializes the POST transfer handler with client data and sets up epoll for reading
 * @param client Reference to the client connection
 * @param bytesRead Number of bytes already read from the client
 * @param buffer Initial buffer containing received data
 */
HandlePostTransfer::HandlePostTransfer(Client &client, size_t bytesRead, string buffer)
: HandleTransfer(client, -1, HANDLE_POST_TRANSFER), _isChunked(false), _foundBoundary(false), _searchContentDisposition(false)
{
    _bytesReadTotal = bytesRead;
    _fileBuffer = buffer;
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
}

/**
 * Assignment operator for HandlePostTransfer
 * Copies all member variables except the client reference (which cannot be reassigned)
 * @param other The HandlePostTransfer object to copy from
 * @return Reference to this object
 */
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

/**
 * Main handler for POST transfer operations
 * Orchestrates the reading of incoming data and processing based on whether it's CGI or regular POST
 * @param readData Flag indicating whether to read new data from the client
 * @return true if the transfer is complete or an error occurred, false if more data is needed
 */
bool HandlePostTransfer::handlePostTransfer(bool readData)
{
    try
    {
        if (readData == true)
            ReadIncomingData();
        if (_client._isCgi)
            return handlePostCgi();
        return processMultipartData();        
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

/**
 * Reads incoming data from the client socket
 * Appends received data to the internal file buffer and updates total bytes read
 */
void HandlePostTransfer::ReadIncomingData()
{
    char buff[RunServers::getClientBufferSize()];
    size_t bytesReceived = RunServers::receiveClientData(_client, buff);
    _bytesReadTotal += bytesReceived;
    _fileBuffer.append(buff, bytesReceived);
}

/**
 * Processes multipart form data from the client
 * Handles boundary detection, content writing, and file management in a loop
 * @return true if processing is complete, false if more data is needed
 */
bool HandlePostTransfer::processMultipartData()
{
    while (true)
    {
        if (_bytesReadTotal > _client._contentLength)
            throw ErrorCodeClientException(_client, 400, "Content length smaller then body received for client with fd: " + to_string(_client._fd));
        if (_searchContentDisposition == true && searchContentDisposition() == false)
            return false;
        if (_foundBoundary == true)
        {
            ValidationResult result = validateFinalCRLF();
            if (result == RERUN_WITHOUT_READING)
                continue;
            return (result == FINISHED); // if result is true, then we are done with the post transfer
        }
        ssize_t bytesWritten = 0;
        size_t boundaryPos = FindBoundaryAndWrite(bytesWritten);
        if (boundaryPos != string::npos)
        {
            _fileBuffer = _fileBuffer.erase(0, _client._bodyBoundary.size() + boundaryPos - bytesWritten);
            RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
            _foundBoundary = true;
        }
        else
            return false;
    }
}


/**
 * Validates the final CRLF sequence after a boundary in multipart data
 * Handles different states: empty line (signals new content), terminator (end of data), or invalid format
 * @return ValidationResult indicating whether to continue reading, finish, or rerun without reading
 */
// return 0 if should continue reading, 1 if should stop reading and finished 2 if should rerun without reading
ValidationResult HandlePostTransfer::validateFinalCRLF()
{
    size_t foundReturn = _fileBuffer.find(CRLF);
    if (foundReturn == 0)
    {
        _fileBuffer.erase(0, CRLF_LEN);
        _searchContentDisposition = true;
        if (_fd != -1)
            FileDescriptor::closeFD(_fd);
        _fd = -1;
        _foundBoundary = false;
        return RERUN_WITHOUT_READING;
    }
    if (foundReturn == BOUNDARY_PREFIX_LEN && strncmp(_fileBuffer.data(), "--\r\n", BOUNDARY_PADDING) == 0 && _fileBuffer.size() <= BOUNDARY_PADDING)
    {
        if (_fd != -1)
            FileDescriptor::closeFD(_fd);
        _fd = -1;
        sendSuccessResponse();
        return FINISHED;
    }
    if (_fileBuffer.size() > TERMINATOR_SIZE)
    {
        std::cout << "filebuffer is after: " << _fileBuffer << std::endl; //testcout
        errorPostTransfer(_client, 400, "post request has more characters then allowed between boundary and return characters");
    }
    return CONTINUE_READING;
}

/**
 * Finds boundary markers in the buffer and writes content to file
 * Handles different boundary formats and writes safe amounts of data to avoid corruption
 * @param bytesWritten Reference to store the number of bytes written to file
 * @return Position of the boundary in the buffer, or string::npos if not found
 */
size_t HandlePostTransfer::FindBoundaryAndWrite(ssize_t &bytesWritten)
{
    size_t boundaryBufferSize = _client._bodyBoundary.size() + BOUNDARY_PADDING; 
    size_t writeSize = (boundaryBufferSize >= _fileBuffer.size()) ? 0 : _fileBuffer.size() - boundaryBufferSize;
    size_t boundaryPos = _fileBuffer.find(_client._bodyBoundary);
    if (boundaryPos != string::npos)
    {
        if (boundaryPos >= BOUNDARY_PADDING && strncmp(_fileBuffer.data() + boundaryPos - BOUNDARY_PADDING, "\r\n--", BOUNDARY_PADDING) == 0)
            writeSize = boundaryPos - BOUNDARY_PADDING;
        else if (strncmp(_fileBuffer.data() + boundaryPos - BOUNDARY_PREFIX_LEN, "--",BOUNDARY_PREFIX_LEN ) == 0)
            writeSize = boundaryPos - BOUNDARY_PREFIX_LEN;
        else
        {
            throw ErrorCodeClientException(_client, 400, "post request has more characters then allowed between content and boundary");
        }
    }
    if (writeSize > 0)
    {
        bytesWritten = write(_fd, _fileBuffer.data(), writeSize);
        if (bytesWritten == -1)
            ErrorCodeClientException(_client, 500, "write failed post request: " + string(strerror(errno)));
        _fileBuffer = _fileBuffer.erase(0, bytesWritten);
    }
    return boundaryPos;
}

/**
 * Searches for and processes Content-Disposition header in multipart data
 * Parses header information, creates output file, and prepares for content writing
 * @return true if header was found and processed, false if more data is needed
 */
bool HandlePostTransfer::searchContentDisposition()
{
    size_t bodyEnd = _fileBuffer.find(CRLF2);
    if (bodyEnd == string::npos)
        return false;
    HttpRequest::getBodyInfo(_client, _fileBuffer);
    _fileBuffer.erase(0, bodyEnd + BOUNDARY_PADDING);
    _fd = open(_client._filenamePath.data(), O_WRONLY | O_TRUNC | O_CREAT, FILE_PERMISSIONS);
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

/* helper functions */

/**
 * Sends a successful HTTP response after POST transfer completion
 * Constructs and sends HTTP 201 Created response with the uploaded file path
 */
void HandlePostTransfer::sendSuccessResponse()
{
    size_t absolutePathSize = RunServers::getServerRootDir().size();
    string relativePath = "." + _client._filenamePath.substr(absolutePathSize) + '\n';
    string headers = HttpRequest::HttpResponse(_client, HTTP_CREATED, ".txt", relativePath.size()) + relativePath;
    
    auto handleClient = make_unique<HandleToClientTransfer>(_client, headers);
    RunServers::insertHandleTransfer(move(handleClient));
    // send(_client._fd, headers.data(), headers.size(), MSG_NOSIGNAL);
    Logger::log(INFO, _client, "POST   ", _client._filenamePath);
}

/**
 * Handles error cleanup and throws an exception for POST transfer errors
 * Closes file descriptors, removes temporary files, and formats error messages
 * @param client Reference to the client connection
 * @param errorCode HTTP error code to return
 * @param errMsg Error message describing the issue
 */
void HandlePostTransfer::errorPostTransfer(Client &client, uint16_t errorCode, string errMsg)
{
    if (_fd != -1)
    {
        FileDescriptor::closeFD(_fd);
        _fd = -1;
    }
    for (const auto &filePath : _fileNamePaths)
    {
        if (remove(filePath.data()) != 0)
            cout << "remove failed on file: " << filePath << endl;
    }
    throw ErrorCodeClientException(client, errorCode, errMsg + " " + strerror(errno));
}

/* cgi handling and checking if correct syntax */

/**
 * Handles CGI POST requests
 * Validates that the multipart data is complete and processes it for CGI execution
 * @return true if CGI processing is successful, false if more data is needed
 */
bool HandlePostTransfer::handlePostCgi()
{
    if (_fileBuffer.find(string(_client._bodyBoundary) + "--" + CRLF) == string::npos)
        return false;

    if (MultipartParser::validateMultipartPostSyntax(_client, _fileBuffer) == true)
    {
        HttpRequest::handleCgi(_client, _fileBuffer);
        return true;
    }
    else
        throw ErrorCodeClientException(_client, 400, "Malformed POST request syntax for CGI");
    return false;
}

/**
 * Validates the syntax of multipart POST data for CGI processing
 * Parses boundaries, headers, and ensures proper multipart format compliance
 * @param client Reference to the client connection
 * @param input String containing the complete multipart data to validate
 * @return true if syntax is valid, throws exception if invalid
 */
bool MultipartParser::validateMultipartPostSyntax(Client &client, string &input)
{
    if (input.size() != client._contentLength)
        throw ErrorCodeClientException(client, 400, "Content-Length does not match body size, " + string("content_length: ") + to_string(client._contentLength) + ", input size: " + to_string(input.size()));
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


/**
 * Parses Content-Disposition header from multipart data
 * Extracts header information and advances the buffer past the header section
 * @param client Reference to the client connection
 * @param buffer Reference to the string_view buffer being processed
 */
void MultipartParser::parseContentDisposition(Client &client, string_view &buffer)
{
    size_t bodyEnd = buffer.find(CRLF2);
    if (bodyEnd == string_view::npos)
        throw ErrorCodeClientException(client, 400, "Missing double CRLF after Content-Disposition");
    string buf = string(buffer.substr(0, bodyEnd));
    HttpRequest::getBodyInfo(client, buf);
    buffer.remove_prefix(bodyEnd + CRLF2_LEN);
}

/**
 * Validates boundary terminator sequences in multipart data
 * Checks for empty lines (content disposition signals) or proper boundary terminators
 * @param client Reference to the client connection
 * @param buffer Reference to the string_view buffer being processed
 * @param needsContentDisposition Reference to flag indicating if content disposition is needed
 * @return true if end of multipart data is reached, false otherwise
 */
bool MultipartParser::validateBoundaryTerminator(Client &client, string_view &buffer, bool &needsContentDisposition)
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
    if (crlfPos == BOUNDARY_PREFIX_LEN && buffer.size() >= terminator.size() && 
        strncmp(buffer.data(), terminator.data(), terminator.size()) == 0)
        return true;
    throw ErrorCodeClientException(client, 400, "invalid post request to cgi)");
}


