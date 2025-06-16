#include <iostream>
#include <FileDescriptor.hpp>
#include <Server.hpp>

#include <unordered_set>
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // close
#include <stdlib.h>	// callod
#ifdef __linux__
#include <sys/epoll.h>
#endif
#include <sys/socket.h>

#include <dirent.h>

#include <signal.h>

#include <fstream>
#include <sstream>
#include <utils.hpp>
#include <sys/stat.h>

using namespace std;

enum ContentType
{
    UNSUPPORTED,
    FORM_URLENCODED,
    JSON,
    TEXT,
    MULTIPART
};


class HttpRequest
{
    public:
        HttpRequest(unique_ptr<Server> &server, int clientFD, string &method, string &header, string &body);

        void	handleRequest(size_t contentLength);

        void	parseHeaders(const string& headerBlock);
        void	getBodyInfo(string &body);

        void	POST();
        void	GET();

        static string  getMimeType(string &path);
        static string HttpResponse(uint16_t code, string path, size_t fileSize);

        void    pathHandling();

        ContentType getContentType(const string_view ct);
		void setLocation();

        
    private:
        unique_ptr<Server> &_server;

        int _clientFD;
        string &_method;
        string &_headerBlock;
        unordered_map<string, string_view> _headers;

        string &_body;
        
        // unordered_map<string, string_view> headers;
        string _hostName;
        string _contentLength;
        string_view _contentType;
        string_view _bodyBoundary;
        
        string _path;
        string_view _filename;
        string_view _fileContent;

		Alocation _location;
};