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

public:
    void handleErrorClient() const;
    explicit ErrorCodeClientException(Client &client, int errorCode, const string &message, map<uint16_t, string> _errorPages);
    const char *what() const throw();
};

#endif