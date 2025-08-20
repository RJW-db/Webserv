#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#define RESERVED_FDS 5         // stdin, stdout, stderr, server, epoll
#define INCOMING_FD_LIMIT 1024 // Makefile, shell(ulimit -n)
// TODO if FD minimum = 5, immediate turning off webserv with error.
// 4th is listener FD, 5th FD is client.
#define FD_LIMIT 1024 - RESERVED_FDS

#define PORT "8080"

#define EPOLL_DEL_EVENTS 0
# define _XOPEN_SOURCE 700  // VSC related, make signal and struct visisible
#include <iostream>

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

        static void getExecutableDirectory();
        static void setupEpoll();

        static void epollInit(ServerList &servers);
        static void addStdinToEpoll();

		static void createServers(vector<ConfigServer> &configs);

        static void runServers();
        static void handleEvents(size_t eventCount);
        static void acceptConnection(const int listener);
        static bool addFdToEpoll(int infd);

        static void setClientServerAddress(Client &client, int infd);

        static void setServerFromListener(Client &client);


        static void processClientRequest(Client &client);
        static size_t receiveClientData(Client &client, char *buff);

        static void serverMatchesPortHost(Server &Server, int fd);
		static void setServerFromHost(Client &client);
        static void setLocation(Client &state);

        static bool makeSocketNonBlocking(int sfd);

        static void setEpollEvents(int fd, int option, uint32_t events);

		static unique_ptr<Client> &getClient(int fd);

        static void parseHeaders(Client &client);
        // unordered_map<string, string_view> _headerFields;

        static void insertHandleTransfer(unique_ptr<HandleTransfer> handle);
        static void insertHandleTransferCgi(unique_ptr<HandleTransfer> handle);

        static void clientHttpCleanup(Client &client);

        static void cleanupServer();
        static void cleanupFD(int &fd);
        static void cleanupClient(Client &client);

        static void checkClientDisconnects();
        static void checkCgiDisconnect();
        static void closeHandles(pid_t pid);
        static void removeHandlesWithFD(int fd);

            static bool runHandleTransfer(struct epoll_event &currentEvent);
        static bool runCgiHandleTransfer(struct epoll_event &currentEvent);

        static inline string &getServerRootDir()
        {
            return _serverRootDir;
        }
        static void setClientBufferSize(uint64_t value)
        {
            _clientBufferSize = value;
        }

        static inline uint64_t getClientBufferSize()
        {
            return _clientBufferSize;
        }
        static inline uint64_t getRamBufferLimit()
        {
            return _ramBufferLimit;
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


        static int getEpollFD()
        {
            return _epfd;
        }

    private:
        // string _serverName;
        // int _listener; // moet weg

		// static FileDescriptor _fds;
        static string _serverRootDir;

        static int _epfd;
        static array<struct epoll_event, FD_LIMIT> _events;

		static ServerList _servers;
        // static unordered_map<int, string> _fdBuffers;
        // static unordered_map<int, ClientRequestState> _clientStates;
        // static vector<int> _connectedClients;
        // static vector<HandleTransfer> _handle;
        static vector<unique_ptr<HandleTransfer>> _handle;
        static vector<unique_ptr<HandleTransfer>> _handleCgi;
        static vector<int> _listenFDS;
        static unordered_map<int, unique_ptr<Client>> _clients;

        static int _level;
        static uint64_t _clientBufferSize;
        static uint64_t _ramBufferLimit;
};

void	poll_usages(void);
void	epoll_usage(void);
int		getaddrinfo_usage(void);
int		server(void);

void parseHttpRequest(string &request);
std::string escapeSpecialChars(const std::string& input);


string  extractMethod(const string &header);
string  extractHeader(const string &header, const string &key);
void    sendErrorResponse(int clientFD, const std::string &message);
void    handleRequest(int clientFD, string &method, string &header, string &body);
#endif
