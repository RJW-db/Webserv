#include <HandleTransfer.hpp>


HandleTransfer::HandleTransfer(int clientFD, string &responseHeader, int fd)
: _clientFD(clientFD), _header(responseHeader), _fd(fd)
{
    _offset = 0;
    _epollout_enabled = false;
}