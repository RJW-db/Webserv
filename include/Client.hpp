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
    HEADER_AWAITING = 0,
    BODY_CHUNKED = 1,
    BODY_AWAITING = 2,
    BODY_READY = 3
};

class Client
{
    public:
            // Add more fields as needed
        Client(int fd) : _fd(fd), _headerParseState(HEADER_AWAITING), _keepAlive(true), _contentLength(0), _chunkPos(0){}
        // Client &operator=(const Client &other);
		
        void resetRequestState();

		void setDisconnectTime(uint16_t disconectTimeSeconds)
		{
			_disconnectTime = chrono::steady_clock::now() + chrono::seconds(disconectTimeSeconds);
		};
        
		int _fd;

        unique_ptr<Server> _usedServer;
        Location _location;
        string_view _uploadPath;

        int8_t _headerParseState;
        string _header;
        string _body;
        string _method;
        uint8_t _useMethod;
        string _requestPath;
        string _rootPath; // root + requestpath + filename
        string _version;
        size_t _contentLength;
        size_t _bodyEnd;
        size_t _chunkPos;
        string_view _contentType;
        string_view _bodyBoundary;

        string_view _filename;
        string _pathFilename;
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
