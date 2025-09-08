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
        void handleErrorClient();

    private:
        //variables
        Client &_client;
        uint16_t _errorCode;
        string _message;
        int _fileFD = -1;
        map<uint16_t, string> _errorPages;

        //helper functions
        void handleDefaultErrorPage() const;
        void handleCustomErrorPage(const string &errorPagePath);

    public:
        // utility function
        const char *what() const throw() {
            return _message.c_str();
        }

        // Getters
        uint16_t getErrorCode() const {
            return _errorCode;
        }
        const string &getMessage() const {
            return _message;
        }
};

#endif