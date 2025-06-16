#ifndef ERRORCODECLIENTEXCEPTION_HPP
# define ERRORCODECLIENTEXCEPTION_HPP

#include <string>
#include <map>
#include <fcntl.h>
#include <utils.hpp>
using namespace std;

class ErrorCodeClientException
{
private:
    int _clientFD;
    uint16_t _errorCode;
    string _message;
    map<uint16_t, string> _errorPages;

public:
    void handleErrorClient() const;
    explicit ErrorCodeClientException(int clientFD, int errorCode, const std::string &message, map<uint16_t, string> _errorPages);
    const char *what() const throw();
};

#endif