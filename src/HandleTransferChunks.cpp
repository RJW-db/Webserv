#include <HandleTransfer.hpp>
#include <RunServer.hpp>
#include <ErrorCodeClientException.hpp>
#include <HttpRequest.hpp>
#include <Client.hpp>

void HandleTransfer::appendToBody()
{
    char   buff[RunServers::getClientBufferSize()];
    size_t bytesReceived = RunServers::receiveClientData(_client, buff);
    _client._body.append(buff, bytesReceived);
}

bool HandleTransfer::handleChunkTransfer()
{
    size_t chunkTargetSize = 0;
    try
    {
        if (decodeChunk(chunkTargetSize) == false)
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
    
    if (chunkTargetSize == 0)
        return true;
    handlePostTransfer(false);
    return false;
}

bool    HandleTransfer::decodeChunk(size_t &chunkTargetSize)
{
    size_t chunkDataStart;

    if (extractChunkSize(chunkTargetSize, chunkDataStart) == false)
    {
        return false;
    }
    string &body = _client._body;
    size_t chunkDataEnd = chunkDataStart + chunkTargetSize;

    if (body.size() >= chunkDataEnd + CRLF_LEN)
    {
        if (body[chunkDataEnd] == '\r' &&
            body[chunkDataEnd + 1] == '\n')
        {
            _bodyPos = chunkDataEnd + CRLF_LEN;
            _fileBuffer.append(body.data() + chunkDataStart, chunkDataEnd - chunkDataStart);
        }
        else
        {
            throw ErrorCodeClientException(_client, 400, "Chunk data missing CRLF ");
        }
    }
    return true;
}

bool HandleTransfer::extractChunkSize(size_t &chunkTargetSize, size_t &chunkDataStart)
{
    size_t crlfPos = _client._body.find(CRLF, _bodyPos);

    if (crlfPos == string::npos)
        return false;

    string chunkSizeLine = _client._body.substr(_bodyPos, crlfPos);
    validateChunkSizeLine(chunkSizeLine);
    chunkTargetSize = parseChunkSize(chunkSizeLine);
    _bytesReadTotal += chunkTargetSize;
    _client._contentLength += chunkTargetSize;
    chunkDataStart = crlfPos + CRLF_LEN;
    return true;
}


void HandleTransfer::validateChunkSizeLine(const string &input)
{
    if (input.size() < 1)
    {
        throw ErrorCodeClientException(_client, 400, "Invalid chunk size given: " + input);
    }

    if (all_of(input.begin(), input.end(), ::isxdigit) == false)
    {
        throw ErrorCodeClientException(_client, 400, "Invalid chunk size given: " + input);
    }

    if (input[input.size() - 2] != '\r' && input[input.size() - 1] != '\n')
    {
        throw ErrorCodeClientException(_client, 400, "Invalid chunk size given: " + input);
    }
}

uint64_t HandleTransfer::parseChunkSize(const string &input)
{
    uint64_t chunkTargetSize;
    stringstream ss;
    ss << hex << input;
    ss >> chunkTargetSize;

    if (static_cast<size_t>(chunkTargetSize) <= _client._location.getClientMaxBodySize())
        return chunkTargetSize;
    throw ErrorCodeClientException(_client, 413, "Content-Length exceeds maximum allowed: " + to_string(chunkTargetSize));
}
