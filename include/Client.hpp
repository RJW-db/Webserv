#ifndef CLIENT_HPP
# define CLIENT_HPP
#include <string>
#include <unordered_map>
#include <string_view>

#include <Server.hpp>
#include <Location.hpp>
#include <chrono>

enum HeaderParseState
{
    HEADER_AWAITING = 0,
    BODY_CHUNKED = 1,
    BODY_AWAITING = 2,
    REQUEST_READY = 3
};

enum HttpMethod : uint8_t
{
    METHOD_HEAD   = 1,
    METHOD_GET    = 2,
    METHOD_POST   = 4,
    METHOD_DELETE = 8
};

class Client
{
    public:
            // Add more fields as needed
        Client(int fd);
        Client(const Client& other)
            : _fd(other._fd),
              _usedServer(other._usedServer ? std::make_unique<Server>(*other._usedServer) : nullptr),
              _location(other._location),
              _uploadPath(other._uploadPath),
              _isCgi(other._isCgi),
              _headerParseState(other._headerParseState),
              _header(other._header),
              _body(other._body),
              _method(other._method),
              _useMethod(other._useMethod),
              _requestPath(other._requestPath),
              _queryString(other._queryString),
              _rootPath(other._rootPath),
              _filenamePath(other._filenamePath),
              _version(other._version),
              _contentLength(other._contentLength),
              _bodyEnd(other._bodyEnd),
              _contentType(other._contentType),
              _bodyBoundary(other._bodyBoundary),
              _filename(other._filename),
              _name(other._name),
              _fileContent(other._fileContent),
              _isAutoIndex(other._isAutoIndex),
              _disconnectTime(other._disconnectTime),
              _keepAlive(other._keepAlive),
              _headerFields(other._headerFields)
        {}
        // Client &operator=(const Client &other);

        void resetRequestState();

		void setDisconnectTime(uint16_t disconectTimeSeconds)
		{
			_disconnectTime = chrono::steady_clock::now() + chrono::seconds(disconectTimeSeconds);
		};
        // bool _finishedProcessClientRequest = false;
		int _fd;

        unique_ptr<Server> _usedServer;
        Location _location;
        string_view _uploadPath;
        bool _isCgi = false;

        int8_t _headerParseState;
        string _header;
        string _body;
        string _method;
        uint8_t _useMethod;
        string _requestPath;
        string _queryString;
        string _rootPath; // root + requestpath
		string _filenamePath; // rootpath + filename
        string _version;
        size_t _contentLength;
        size_t _bodyEnd;
        string_view _contentType;
        string_view _bodyBoundary;  // shorten to just _boundary

        pid_t _pid = -1;
        string _filename;
        string _name;
        string_view _fileContent;

        bool _isAutoIndex = false;
  

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
