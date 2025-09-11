#include <sys/epoll.h>
#include <fcntl.h>
#include "ErrorCodeClientException.hpp"
#include "HandleTransfer.hpp"
#include "FileDescriptor.hpp"
#include "HttpRequest.hpp"
#include "RunServer.hpp"
#include "Logger.hpp"
namespace
{
    constexpr const char ERR400[] =
    "<html>\n"
    "<head><title>400 Bad Request</title></head>\n"
    "<body>\n"
    "    <h1>400 Bad Request</h1>\n"
    "    <p>Your browser sent a request that this server could not understand.</p>\n"
    "</body>\n"
    "</html>";

    constexpr const char ERR500[] =
    "<html>\n"
    "<head><title>500 Internal Server Error</title></head>\n"
    "<body>\n"
    "    <h1>500 Internal Server Error</h1>\n"
    "    <p>The server encountered an internal error and was unable to complete your request.</p>\n"
    "</body>\n"
    "</html>";
}

ErrorCodeClientException::ErrorCodeClientException(Client &client, uint16_t errorCode, const string &message)
: _client(client), _errorCode(errorCode), _message(message), _errorPages(client._location.getErrorCodesWithPage())
{
    // _errorPages = client._location.getErrorCodesWithPage();
}

void ErrorCodeClientException::handleErrorClient()
{
    try {
        Logger::log(IWARN, _client, _message);
        _client._keepAlive = false;
        if (_errorCode == 0) {
            RunServers::cleanupClient(_client);
            return;
        }
        map<uint16_t, std::string>::const_iterator errorPageIt = _errorPages.find(_errorCode);
        if (errorPageIt == _errorPages.end()) {
            handleDefaultErrorPage();
            return;
        }
        handleCustomErrorPage(errorPageIt->second);
        return;
    }
    catch (const exception& e) {
        Logger::log(ERROR, "Server error", '-', "Exception in handleErrorClient: ", e.what());
    }
    catch (ErrorCodeClientException &e) {
        Logger::log(ERROR, "Server error", '-', "Nested ErrorCodeClientException: ", e.what());
    }
    catch (...) {
        Logger::log(ERROR, "Server error", '-', "Unknown exception in handleErrorClient");
    }
    int closeFd = _fileFD;
    FileDescriptor::closeFD(closeFd);
    RunServers::cleanupClient(_client);
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
    else {
        body = "<html>\n"
            "  <head><title>" + to_string(_errorCode) + " Error</title></head>\n"
            "  <body>\n"
            "    <h1>" + to_string(_errorCode) + " Error</h1>\n"
            "    <p>An error occurred (" + to_string(_errorCode) + ").</p>\n"
            "  </body>\n"
            "</html>";
    }
    message = HttpRequest::HttpResponse(_client, _errorCode, ".html", body.size());
    message += body;
    unique_ptr handleClient = make_unique<HandleToClientTransfer>(_client, message);
    RunServers::insertHandleTransfer(move(handleClient));
}

void ErrorCodeClientException::handleCustomErrorPage(const string &errorPagePath)
{
    _fileFD = open(errorPagePath.c_str(), O_RDONLY);
    if (_fileFD == -1) {
        Logger::log(ERROR, "Server error", '-', "Failed to open error page: ", errorPagePath);
        handleDefaultErrorPage();
        return;
    }
    FileDescriptor::setFD(_fileFD);
    Logger::log(INFO, "Serving custom error page", _fileFD, "errorPageFD");
    size_t fileSize = getFileLength(_client, string_view(errorPagePath));
    _client._filenamePath = errorPagePath;
    string response = HttpRequest::HttpResponse(_client, _errorCode, errorPagePath, fileSize);
    auto transfer = make_unique<HandleGetTransfer>(_client, _fileFD, response, fileSize, response.size());
    RunServers::insertHandleTransfer(move(transfer));
}
