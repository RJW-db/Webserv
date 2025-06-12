#include <HandleTransfer.hpp>


// HandleTransfer::HandleTransfer(int clientFD, string &responseHeader, int fd)
// : _clientFD(clientFD), _header(responseHeader), _fd(fd)
HandleTransfer::HandleTransfer(int clientFD, string &responseHeader, int fd, size_t fileSize, string &boundary)
: _clientFD(clientFD), _fd(fd), _fileSize(fileSize), _fileBuffer(responseHeader), _boundary(boundary)
{
    _offset = 0;
    _bytesReadTotal = 0;
    _epollout_enabled = false;
}