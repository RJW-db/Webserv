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


void ErrorCodeClientException::handleDefaultErrorPage() const
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
    {
        body = "<html>\n"
            "  <head><title>" + std::to_string(_errorCode) + " Error</title></head>\n"
            "  <body>\n"
            "    <h1>" + std::to_string(_errorCode) + " Error</h1>\n"
            "    <p>An error occurred (" + std::to_string(_errorCode) + ").</p>\n"
            "  </body>\n"
            "</html>";
        message = HttpRequest::HttpResponse(_client, _errorCode, ".html", body.size());
    }
    message += body;
    auto handleClient = make_unique<HandleToClientTransfer>(_client, message);
    RunServers::insertHandleTransfer(move(handleClient));
    RunServers::cleanupClient(_client);
}

void ErrorCodeClientException::handleCustomErrorPage(const std::string& errorPagePath, int errorCode) const
{
    int fd = open(errorPagePath.c_str(), O_RDONLY);
    if (fd == -1)
    {
        RunServers::cleanupClient(_client);
        throw runtime_error("Couldn't open errorpage: " + errorPagePath);
    }
    size_t fileSize = getFileLength(errorPagePath.c_str());
    _client._filenamePath = errorPagePath;
    FileDescriptor::setFD(fd);
    string response = HttpRequest::HttpResponse(_client, errorCode, errorPagePath, fileSize);
    auto transfer = make_unique<HandleGetTransfer>(_client, fd, response, fileSize);
    RunServers::insertHandleTransfer(move(transfer));
}

void ErrorCodeClientException::handleErrorClient() const
{
    Logger::log(IWARN, _client, _message);
    if (_errorCode == 0)
    {
        RunServers::cleanupClient(_client);
        return;
    }
    auto it = _errorPages.find(_errorCode);
    if (it == _errorPages.end())
    {
        handleDefaultErrorPage();
        return;
    }
    handleCustomErrorPage(it->second, it->first);
}

uint16_t ErrorCodeClientException::getErrorCode() const { return _errorCode; }
string ErrorCodeClientException::getMessage() const { return _message; }
