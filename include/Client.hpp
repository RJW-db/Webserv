#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <unordered_map>
#include <string_view>
#include <string>
#include <memory>
#include <chrono>
#include "ConfigServer.hpp"
using namespace std;
namespace
{
    enum HeaderParseState : uint8_t
    {
        HEADER_AWAITING = 0,
        BODY_CHUNKED = 1,
        BODY_AWAITING = 2,
        REQUEST_READY = 3
    };

    enum HttpMethod : uint8_t
    {
        METHOD_INVALID = 0,
        METHOD_HEAD = 1,
        METHOD_GET = 2,
        METHOD_POST = 4,
        METHOD_DELETE = 8
    };
}

class Client
{
    public:
        explicit Client(int fd);

        void httpCleanup();

        void setDisconnectTime(uint16_t disconectTimeSeconds)
        {
            _disconnectTime = chrono::steady_clock::now() + chrono::seconds(disconectTimeSeconds);
        };
        void setDisconnectTimeCgi(uint16_t disconectTimeSeconds)
        {
            _disconnectTimeCgi = chrono::steady_clock::now() + chrono::seconds(disconectTimeSeconds);
        };

        int _fd;
        unique_ptr<AconfigServ> _usedServer;
        pair<string, string> _ipPort;
        Location _location;
        string_view _uploadPath;
        string _sessionId;
        bool _isCgi = false;

        uint8_t _headerParseState = HEADER_AWAITING;
        string _header;
        string _body;
        string _method;
        uint8_t _useMethod = 0;
        string _requestPath;
        bool _requestUpload = false;
        string _queryString;
        string _rootPath;      // root + requestpath
        string _filenamePath;  // rootpath + filename
        string _version;
        size_t _contentLength = 0;
        size_t _bodyEnd = 0;
        string_view _contentType;
        string_view _boundary;

        pid_t _pid = -1;
        string _filename;
        string _name;

        bool    _cgiClosing = false;
        chrono::steady_clock::time_point _disconnectTimeCgi;

        bool _isAutoIndex = false;

        chrono::steady_clock::time_point _disconnectTime;
        bool _keepAlive = true;
        unordered_map<string, string_view> _headerFields;
};
#endif
