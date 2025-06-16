#include <ErrorCodeClientException.hpp>
#include <HandleTransfer.hpp>
#include <FileDescriptor.hpp>
#include <RunServer.hpp>
#include <sys/epoll.h>

ErrorCodeClientException::ErrorCodeClientException(int clientFD, int errorCode, const std::string &message, map<uint16_t, string> _errorPages)
: _clientFD(clientFD), _errorCode(errorCode), _message(message), _errorPages(_errorPages) 
{

}

const char *ErrorCodeClientException::what() const throw()
{
    return _message.c_str();
}

void ErrorCodeClientException::handleErrorClient() const
{
    auto it = _errorPages.find(_errorCode);
    if (it == _errorPages.end())
        throw runtime_error("invalid error code given in code: " + to_string(_errorCode));
    int fd = open(it->second.c_str(), O_RDONLY);
    size_t fileSize = getFileLength(it->second.c_str());
    FileDescriptor::setFD(fd);
    string empty;
    RunServers::setEpollEvents(_clientFD, EPOLL_CTL_MOD, EPOLLIN | EPOLLOUT);
    auto transfer = make_unique<HandleTransfer>(_clientFD, empty, fd, fileSize);
    RunServers::insertHandleTransfer(move(transfer));
    std::cerr << _message << std::endl;
}
