#include <Client.hpp>

Client::Client(int fd)
: _fd(fd), _headerParseState(HEADER_AWAITING), _contentLength(0), _keepAlive(true)
{}

void Client::resetRequestState()
{
    _headerParseState = HEADER_AWAITING;
    _header.clear();
    _body.clear();
    _method.clear();
    _requestPath.clear();
    _queryString.clear();
    _rootPath.clear();
    _version.clear();
    _contentLength = 0;
    _contentType = string_view();
    _bodyBoundary = string_view();
    _filename.clear();
    _filenamePath.clear();
    _fileContent = string_view();
    _headerFields.clear();
    _isCgi = false;
    _cgiPid = -1;
    // Do NOT reset: _fd, _usedServer, _location, _uploadPath, _disconnectTime, _keepAlive
}