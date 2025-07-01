#include <ErrorCodeClientException.hpp>
#include <HandleTransfer.hpp>
#include <HttpRequest.hpp>
#include <FileDescriptor.hpp>
#include <RunServer.hpp>
#include <sys/epoll.h>

ErrorCodeClientException::ErrorCodeClientException(Client &client, int errorCode, const std::string &message)
: _client(client), _errorCode(errorCode), _message(message)
{
    _errorPages = client._location.getErrorCodesWithPage();
}

const char *ErrorCodeClientException::what() const throw()
{
    return _message.c_str();
}

void ErrorCodeClientException::handleErrorClient() const
{
    std::cerr << _message << std::endl;
    if (_errorCode == 0)
    {
        // RunServers::cleanupClient(_client);
        return ;
    }
    auto it = _errorPages.find(_errorCode);
    if (it == _errorPages.end())
        throw runtime_error("invalid error code given in code: " + to_string(_errorCode));
    int fd = open(it->second.c_str(), O_RDONLY);
    size_t fileSize = getFileLength(it->second.c_str());
    FileDescriptor::setFD(fd);
    string response = HttpRequest::HttpResponse(_client, it->first, it->second, fileSize);
    auto transfer = make_unique<HandleTransfer>(_client, fd, response, fileSize);
    // _client._keepAlive = false; // TODO: check if this is needed
    RunServers::insertHandleTransfer(move(transfer));
}
