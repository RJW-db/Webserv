#ifndef ERRORCODECLIENTEXCEPTION_HPP
# define ERRORCODECLIENTEXCEPTION_HPP

#include <Client.hpp>

#include <string>
#include <map>
#include <fcntl.h>
#include <utils.hpp>
using namespace std;

class ErrorCodeClientException
{
    private:
        Client &_client;
        uint16_t _errorCode;
        string _message;
        map<uint16_t, string> _errorPages;

    void handleDefaultErrorPage() const;
    void handleCustomErrorPage(const string &errorPagePath, int errorCode) const;


public:
    void handleErrorClient() const;
    explicit ErrorCodeClientException(Client &client, int errorCode, const std::string &message);
    const char *what() const throw();
    uint16_t getErrorCode() const;
    string getMessage() const;
};

#endif