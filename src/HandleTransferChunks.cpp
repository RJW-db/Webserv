#include <HandleTransfer.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>
#include <Client.hpp>

HandleChunkTransfer::HandleChunkTransfer(Client &client)
: HandlePostTransfer(client, -1)
{
    _isChunked = true;
    _bytesReadTotal = 0;
    RunServers::setEpollEvents(_client._fd, EPOLL_CTL_MOD, EPOLLIN);
}

void HandleChunkTransfer::appendToBody()
{
    char   buff[RunServers::getClientBufferSize()];
    size_t bytesReceived = RunServers::receiveClientData(_client, buff);
    _client._body.append(buff, bytesReceived);
}

bool HandleChunkTransfer::handleChunkTransfer()
{
    size_t targetSize = 0;
    try
    {
        if (decodeChunk(targetSize) == false)
            return false;
    }
    catch(const exception& e)
    {
        _client._keepAlive = false;
        errorPostTransfer(_client, 500, "Error in handlePostTransfer: " + string(e.what()));
    }
    catch (const ErrorCodeClientException &e)
    {
        _client._keepAlive = false;
        errorPostTransfer(_client, e.getErrorCode(), e.getMessage());
    }
    
    if (_completedRequest == true) // doing it this way check _fileBuffer.size() < MAX ALLOWED SIZE
    {
        if (_client._isCgi == false)
            handlePostTransfer(false);
        else
        {
            validateMultipartPostSyntax(_client, _fileBuffer);
            HttpRequest::handleCgi(_client, _fileBuffer);
        }
        return true;
    }
    return false;
}

bool    HandleChunkTransfer::decodeChunk(size_t &targetSize)
{
    while (true)
    {
        size_t dataStart;

        if (extractChunkSize(targetSize, dataStart) == false)
            return false;

        string &body = _client._body;
        size_t dataEnd = dataStart + targetSize;
        
        if (body.size() >= dataEnd + CRLF_LEN)
        {
            if (body[dataEnd] == '\r' &&
                body[dataEnd + 1] == '\n')
            {
                if (targetSize == 0 &&
                    body[dataEnd - 1] == '\n' && body[dataEnd - 2] == '\r')
                {
                    _completedRequest = true;
                    return true;
                }
                _bodyPos = dataEnd + CRLF_LEN;
                _fileBuffer.append(body.data() + dataStart, dataEnd - dataStart);
            }
            else
            {
                throw ErrorCodeClientException(_client, 400, "Chunk data missing CRLF ");
            }
            bool exceedsMaxSize = (_fileBuffer.size() > RunServers::getRamBufferLimit());
            if (_client._isCgi == true && exceedsMaxSize == true)
                throw ErrorCodeClientException(_client, 413, "Chunk data missing CRLF ");
            else if (exceedsMaxSize == true)
                handlePostTransfer(false);
        }
        else
            return false;
    }
    return false; // shouldn't never get here
}

bool HandleChunkTransfer::extractChunkSize(size_t &targetSize, size_t &dataStart)
{
    size_t crlfPos = _client._body.find(CRLF, _bodyPos);

    if (crlfPos == string::npos)
        return false;

    string_view chunkSizeLine(_client._body.data() + _bodyPos, crlfPos - _bodyPos);

    validateChunkSizeLine(chunkSizeLine);
    targetSize = parseChunkSize(chunkSizeLine);
    _bytesReadTotal += targetSize;
    _client._contentLength += targetSize;
    dataStart = crlfPos + CRLF_LEN;
    return true;
}

void HandleChunkTransfer::validateChunkSizeLine(string_view chunkSizeLine)
{
    if (chunkSizeLine.empty())
        throw ErrorCodeClientException(_client, 400, "Chunk size line is empty: " + string(chunkSizeLine));

    if (all_of(chunkSizeLine.begin(), chunkSizeLine.end(), ::isxdigit) == false)
        throw ErrorCodeClientException(_client, 400, "Chunk size line contains non-hex characters: " + string(chunkSizeLine));
}

uint64_t HandleChunkTransfer::parseChunkSize(string_view chunkSizeLine)
{
    uint64_t targetSize;
    stringstream ss;
    ss << hex << chunkSizeLine;
    ss >> targetSize;

    if (static_cast<size_t>(targetSize) <= _client._location.getClientMaxBodySize())
        return targetSize;
    throw ErrorCodeClientException(_client, 413, "Content-Length exceeds maximum allowed: " + to_string(targetSize));
}
