#include <Client.hpp>

Client::Client(int fd)
: _fd(fd), _headerParseState(HEADER_AWAITING), _keepAlive(true), _contentLength(0), _bodyPos(0),
_chunkBodyPos(0), _chunkTargetSize(0), _chunkReceivedSize(0)
{}

void Client::resetRequestState()
{
    _headerParseState = HEADER_AWAITING;
    _header.clear();
    _body.clear();
    _method.clear();
    _requestPath.clear();
    _rootPath.clear();
    _version.clear();
    _contentLength = 0;
    _contentType = string_view();
    _bodyBoundary = string_view();
    _filename = string_view();
    _filenamePath.clear();
    _fileContent = string_view();
    _headerFields.clear();
    // Do NOT reset: _fd, _usedServer, _location, _uploadPath, _disconnectTime, _keepAlive
}