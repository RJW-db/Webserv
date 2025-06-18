#ifndef CLIENT_HPP
# define CLIENT_HPP
#include <string>
#include <unordered_map>
#include <string_view>

class Client
{
    public:
        int _fd;
        bool _headerParsed = false;
        string _header;
        string _body;
        string _path;
        string _method;
        size_t _contentLength = 0;
        
        string _fdBuffers;
        unordered_map<string, string_view> _headerFields;
        // Add more fields as needed
        Client(int fd) : _fd(fd) {}
};

#endif
