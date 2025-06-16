#include <HandleTransfer.hpp>


// HandleTransfer::HandleTransfer(int clientFD, string &responseHeader, int fd)
// : _clientFD(clientFD), _header(responseHeader), _fd(fd)
HandleTransfer::HandleTransfer(int clientFD, string &responseHeader, int fd, size_t fileSize)
: _clientFD(clientFD), _fd(fd), _fileSize(fileSize), _fileBuffer(responseHeader)
{
    _offset = 0;
    _bytesReadTotal = 0;
    _epollout_enabled = false;
    _headerSize = responseHeader.size();
}