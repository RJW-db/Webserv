#include <ErrorCodeClientException.hpp>
#include <HandleTransfer.hpp>
#include <HttpRequest.hpp>
#include <FileDescriptor.hpp>
#include <RunServer.hpp>
#include <sys/epoll.h>
#define ERR400 \
"<html>\n\
  <head><title>400 Bad Request</title></head>\n\
  <body>\n\
    <h1>400 Bad Request</h1>\n\
    <p>Your browser sent a request that this server could not understand.</p>\n\
  </body>\n\
</html>"
#define ERR500 \
"<html>\n\
  <head><title>500 Internal Server Error</title></head>\n\
  <body>\n\
    <h1>500 Internal Server Error</h1>\n\
    <p>The server encountered an internal error and was unable to complete your request.</p>\n\
  </body>\n\
</html>"

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
        // RunServers::clientHttpCleanup(_client);
        RunServers::cleanupClient(_client);
        return ;
    }
    auto it = _errorPages.find(_errorCode);
    if (it == _errorPages.end())
    {
        string message;
        string body;
        _client._keepAlive = false;
        if (_errorCode == 400)
        {
            body = ERR400;
            message = HttpRequest::HttpResponse(_client, 400, ".html", body.size());
        }
        else if (_errorCode == 500)
        {
            body = ERR500;
            message = HttpRequest::HttpResponse(_client, 500, ".html", body.size());
        }
        else
            throw runtime_error("invalid error code given in code: " + to_string(_errorCode));
        message += body;
        send(_client._fd, message.data(), message.size(), 0);
        RunServers::cleanupClient(_client);
        return;
    }
    int fd = open(it->second.c_str(), O_RDONLY);
    size_t fileSize = getFileLength(it->second.c_str());
    FileDescriptor::setFD(fd);
    string response = HttpRequest::HttpResponse(_client, it->first, it->second, fileSize);
    auto transfer = make_unique<HandleTransfer>(_client, fd, response, fileSize);
    // _client._keepAlive = false; // TODO: check if this is needed
    RunServers::insertHandleTransfer(move(transfer));
}
