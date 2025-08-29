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
: _client(client), _errorCode(static_cast<uint16_t>(errorCode)), _message(message)
{
    _errorPages = client._location.getErrorCodesWithPage();
}

void ErrorCodeClientException::handleErrorClient() const
{
    try
    {
        Logger::log(IWARN, _client, _message);
        _client._keepAlive = false;
        if (_errorCode == 0)
        {
            RunServers::cleanupClient(_client);
            return;
        }
        map<uint16_t, std::string>::const_iterator errorPageIt = _errorPages.find(_errorCode);
        if (errorPageIt == _errorPages.end())
        {
            handleDefaultErrorPage();
            return;
        }
        handleCustomErrorPage(errorPageIt->second);
    }
    catch(const std::exception& e)
    {
        RunServers::cleanupClient(_client);
        Logger::log(ERROR, "Server error", '-', "Exception in handleErrorClient: ", e.what());
        
    }
}

void ErrorCodeClientException::handleDefaultErrorPage() const
{
    string message;
    string body;
    _client._keepAlive = false;
    if (_errorCode == 400)
        body = ERR400;
    else if (_errorCode == 500)
        body = ERR500;
    else
    {
        body = "<html>\n"
            "  <head><title>" + std::to_string(_errorCode) + " Error</title></head>\n"
            "  <body>\n"
            "    <h1>" + std::to_string(_errorCode) + " Error</h1>\n"
            "    <p>An error occurred (" + std::to_string(_errorCode) + ").</p>\n"
            "  </body>\n"
            "</html>";
    }
    message = HttpRequest::HttpResponse(_client, _errorCode, ".html", body.size());
    message += body;
    unique_ptr handleClient = make_unique<HandleToClientTransfer>(_client, message);
    RunServers::insertHandleTransfer(move(handleClient));
}

void ErrorCodeClientException::handleCustomErrorPage(const string &errorPagePath) const
{
    int fd = open(errorPagePath.c_str(), O_RDONLY);
    if (fd == -1)
    {
        Logger::log(ERROR, "Server error", '-', "Failed to open error page: ", errorPagePath);
        handleDefaultErrorPage();
        return;
    }
    FileDescriptor::setFD(fd);
    size_t fileSize = getFileLength(_client, errorPagePath.c_str());
    _client._filenamePath = errorPagePath;
    string response = HttpRequest::HttpResponse(_client, _errorCode, errorPagePath, fileSize);
    auto transfer = make_unique<HandleGetTransfer>(_client, fd, response, fileSize);
    RunServers::insertHandleTransfer(move(transfer));
}

const char *ErrorCodeClientException::what() const throw()
{
    return _message.c_str();
}

uint16_t ErrorCodeClientException::getErrorCode() const { return _errorCode; }
string ErrorCodeClientException::getMessage() const { return _message; }
