#include <RunServer.hpp>
#include <iostream>
#include <FileDescriptor.hpp>

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
#include <sys/stat.h>

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
		int _clientFD;
		string &_headerBlock;
	    unordered_map<string, string_view> _headers;

		string &_method;
		string &_body;

		// unordered_map<string, string_view> headers;
		string _hostName;
		string _contentLength;
		string_view _contentType;
		string_view _bodyBoundary;

		string_view _filename;
		string_view _file;

		HttpRequest(int clientFD, string &method, string &header, string &body);

		void	handleRequest();

    	void	parseHeaders(const string& headerBlock);
		void	getBodyInfo(string &body);

		void	POST();
		void	GET();

		ContentType getContentType(const string_view ct);
};