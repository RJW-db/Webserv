#include "HttpRequest.hpp"
#include "Constants.hpp"
#include "Client.hpp"

Client::Client(int fd)
: _fd(fd),_contentType(UNSUPPORTED)
{}

void Client::httpCleanup()
{
    // Do NOT reset: _fd, _usedServer, _location, _uploadPath, _disconnectTime, _keepAlive
    _headerParseState = HEADER_AWAITING;
    _header.clear();
    _body.clear();
    _boundary = string_view();
    _requestPath.clear();
    _queryString.clear();
    _method.clear();
    _useMethod = 0;
    _contentLength = 0;
    _headerFields.clear();
    _rootPath.clear();
    _filenamePath.clear();
    _contentType = UNSUPPORTED;
    _name.clear();
    _version.clear();
    _bodyEnd = 0;
    _filename.clear();
    setDisconnectTime(DISCONNECT_DELAY_SECONDS);
    _isAutoIndex = false;
    _isCgi = false;
    _pid = -1;
}
