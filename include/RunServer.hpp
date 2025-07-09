#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#define RESERVED_FDS 5         // stdin, stdout, stderr, server, epoll
#define INCOMING_FD_LIMIT 1024 // Makefile, shell(ulimit -n)
// TODO if FD minimum = 5, immediate turning off webserv with error.
// 4th is listener FD, 5th FD is client.
#define FD_LIMIT 1024 - RESERVED_FDS

#define CLIENT_BUFFER_SIZE 8000
#define PORT "8080"

# define _XOPEN_SOURCE 700  // VSC related, make signal and struct visisible
#include <iostream>
#include <sstream> // logMessage

#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <string_view>

#include <Server.hpp>
#include <HandleTransfer.hpp>
// #include <utils.hpp>
#include <Client.hpp>
#include <signal.h>


using namespace std;


extern volatile sig_atomic_t g_signal_status;
// #include <FileDescriptor.hpp>
class FileDescriptor;

using ServerList = vector<unique_ptr<Server>>;

class RunServers
{
    public:
		RunServers() = default;
        // RunServers(tmp_t *serverConf);
        ~RunServers();

        static int epollInit(/* ServerList &servers */);
        static void addStdinToEpoll();

		static void createServers(vector<ConfigServer> &configs);
        // int run(FileDescriptor& fds);
        // static int runServers(vector<Server>& servers);
        static int runServers();
        static void handleEvents(size_t eventCount);
        static void acceptConnection(const int listener);
        static void processClientRequest(Client &client);
        static size_t receiveClientData(Client &client, char *buff);


		static void setServer(Client &client);
        static void setLocation(Client &state);

        static int make_socket_non_blocking(int sfd);

        static void setEpollEvents(int fd, int option, uint32_t events);

        static bool handleGetTransfer(HandleTransfer &client);
		static bool handlePostTransfer(HandleTransfer &handle);
        // static bool handlingSend(HandleTransfer &ht);

		static unique_ptr<Client> &getClient(int fd);

        static void parseHeaders(Client &client);
        unordered_map<string, string_view> _headerFields;

        static void insertHandleTransfer(unique_ptr<HandleTransfer> handle);
        static void insertClientFD(int fd);

        static void clientHttpCleanup(Client &client);

        static void cleanupServer();
        static void cleanupFD(int fd);
        static void cleanupClient(Client &client);

        static bool runHandleTransfer(struct epoll_event &currentEvent);

        static void setClientBufferSize(uint64_t value)
        {
            _clientBufferSize = value;
        }


        template<typename... Args>
        static void logMessage(int arg, Args&&... args)
        {
            if (_level == -1 || arg >= _level)
            {
                std::ostringstream oss;
                (oss << ... << args);
                std::cout << oss.str() << std::endl;
            }
        }
        class ClientException : public exception
		{
            private:
                string _message;
			public:
                explicit ClientException(const string &message) : _message(message) {}
                virtual const char* what() const throw() {
                    return _message.c_str();
                }
		};

        class LengthRequiredException : public ClientException
        {
            public:
                explicit LengthRequiredException(const std::string &message)
                    : ClientException(message) {}
        };

    private:
        // string _serverName;
        // int _listener; // moet weg

		// static FileDescriptor _fds;
        static int _epfd;
        static array<struct epoll_event, FD_LIMIT> _events;
		
		static ServerList _servers;
        // static unordered_map<int, string> _fdBuffers;
        // static unordered_map<int, ClientRequestState> _clientStates;
        // static vector<int> _connectedClients;
        // static vector<HandleTransfer> _handle;
        static vector<unique_ptr<HandleTransfer>> _handle;

        static unordered_map<int, unique_ptr<Client>> _clients;

        static int _level;
        static uint64_t _clientBufferSize;
};

void	poll_usages(void);
void	epoll_usage(void);
int		getaddrinfo_usage(void);
int		server(void);

void parseHttpRequest(string &request);
std::string escape_special_chars(const std::string& input);

void httpRequestLogger(string str);

string  extractMethod(const string &header);
string  extractHeader(const string &header, const string &key);
void    sendErrorResponse(int clientFD, const std::string &message);
void    handleRequest(int clientFD, string &method, string &header, string &body);
#endif
