#ifndef CLIENT_HPP
# define CLIENT_HPP
#include <string>
#include <unordered_map>
#include <string_view>

#include <Server.hpp>
#include <Location.hpp>

class Client
{
    public:
            // Add more fields as needed
        Client(int fd) : _fd(fd) {}
        // Client &operator=(const Client &other);

        int _fd;

        unique_ptr<Server> _usedServer;
        Location _location;

        bool _headerParsed = false;
        string _header;
        string _body;
        string _path;
        string _method;
        size_t _contentLength = 0;
        
        string _fdBuffers;
        unordered_map<string, string_view> _headerFields;

};

// Client &Client::operator=(const Client &other)
// {
//     if (this != &other)
//     {
//         _fd = other._fd;
//         _usedServer = move(other._usedServer); 
//         _location = other._location;
//         _headerParsed = other._headerParsed;
//         _header = other._header;
//         _body = other._body;
//         _path = other._path;
//         _method = other._method;
//         _contentLength = other._contentLength;
//         _headerFields = other._headerFields;
//     }
//     return *this;
// }
#endif
