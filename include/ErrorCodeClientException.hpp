#ifndef ERRORCODECLIENTEXCEPTION_HPP
#define ERRORCODECLIENTEXCEPTION_HPP
#include <string>
#include "utils.hpp"
#include <fcntl.h>
#include <map>
#include "Client.hpp"
#include "Logger.hpp"
using namespace std;

class ErrorCodeClientException
{
    public:
        //initialization
        explicit ErrorCodeClientException(Client &client, int errorCode, const std::string &message);

        //handle error page
        void handleErrorClient() const;


    private:
        //variables
        Client &_client;
        uint16_t _errorCode;
        string _message;
        map<uint16_t, string> _errorPages;

        //helper functions
        void handleDefaultErrorPage() const;
        void handleCustomErrorPage(map<uint16_t, std::string>::const_iterator it, int fd) const;
    public:
        // utility function
        const char *what() const throw();

        //getters
        uint16_t getErrorCode() const;
        string getMessage() const;
};

#endif