#ifndef CLIENT_HPP
# define CLIENT_HPP
#include <string>
#include <unordered_map>
#include <string_view>

#include <Server.hpp>
#include <Location.hpp>
#include <chrono>

#define disconnectDelaySeconds 5

enum HeaderParseState {
    HEADER_NOT_PARSED = 0,
    HEADER_PARSED_POST = 1,
    HEADER_PARSED_NON_POST = 2
};

class Client
{
    public:
            // Add more fields as needed
        Client(int fd) : _fd(fd), _headerParseState(HEADER_NOT_PARSED), _keepAlive(true) {}
        // Client &operator=(const Client &other);
		
		void setDisconnectTime(uint16_t disconectTimeSeconds)
		{
			_disconnectTime = chrono::steady_clock::now() + chrono::seconds(disconectTimeSeconds);
		};
        
		int _fd;

        unique_ptr<Server> _usedServer;
        Location _location;

        int8_t _headerParseState;
        string _header;
        string _body;
        string _method;
        string _path;
        string _version;
        size_t _contentLength = 0;

        string_view _contentType;
        string_view _bodyBoundary;

        string_view _filename;
        string_view _fileContent;

        chrono::steady_clock::time_point _disconnectTime;
		bool _keepAlive;

        unordered_map<string, string_view> _headerFields;
};



// Client &Client::operator=(const Client &other)
// {
//     if (this != &other)
//     {
//         _fd = other._fd;
//         _usedServer = move(other._usedServer); 
//         _location = other._location;
//         _headerParseState = other._headerParseState;
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
