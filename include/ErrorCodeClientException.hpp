#ifndef ERRORCODECLIENTEXCEPTION_HPP
#define ERRORCODECLIENTEXCEPTION_HPP
#include <string>
#include <map>
#include <cstdint>
using namespace std;
class Client;

class ErrorCodeClientException
{
    public:
        // Initialization
        explicit ErrorCodeClientException(Client &client, uint16_t errorCode, const string &message);

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
        void handleCustomErrorPage(const string &errorPagePath) const;

    public:
        // utility function
        const char *what() const throw();

        // Getters
        uint16_t getErrorCode() const;
        string getMessage() const;
};

#endif