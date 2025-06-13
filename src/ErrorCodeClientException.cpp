#include <ErrorCodeClientException.hpp>
#include <HandleTransfer.hpp>
#include <FileDescriptor.hpp>
#include <RunServer.hpp>

ErrorCodeClient::ErrorCodeClient(int clientFD, int errorCode, const std::string &message, map<uint16_t, string> _errorPages)
: _clientFD(clientFD), _errorCode(errorCode), _message(message), _errorPages(_errorPages) 
{

}

const char *ErrorCodeClient::what() const throw()
{
    return _message.c_str();
}

void ErrorCodeClient::handleErrorClient() const
{
    auto it = _errorPages.find(_errorCode);
    if (it == _errorPages.end())
        throw runtime_error("invalid error code given in code: " + to_string(_errorCode));

    int fd = open(it->second.c_str(), O_RDONLY);
    size_t fileSize = getFileLength(it->second.c_str());
    FileDescriptor::setFD(fd);
    string empty;
    HandleTransfer transfer(_clientFD, empty, fd, fileSize);
    std::cout << _message << std::endl;
}

