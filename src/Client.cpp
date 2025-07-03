#include <Client.hpp>

void Client::resetRequestState()
{
    _headerParseState = HEADER_NOT_PARSED;
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
    _pathFilename.clear();
    _fileContent = string_view();
    _headerFields.clear();
    // Do NOT reset: _fd, _usedServer, _location, _uploadPath, _disconnectTime, _keepAlive
}