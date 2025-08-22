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

#include <signal.h> // sig_atomic_t

// #include <memory>
// #include <vector>
// #include <array>
// #include <unordered_map>
// #include <string_view>

#include "Server.hpp"
#include "HandleTransfer.hpp"
#include "Client.hpp"

using namespace std;

extern volatile sig_atomic_t g_signal_status;
// #include <FileDescriptor.hpp>
class FileDescriptor;

using ServerList = vector<unique_ptr<Server>>;

class RunServers
{
    public:
        // initialization
        static void   getExecutableDirectory();
		static void   createServers(vector<ConfigServer> &configs);
        static void   setupEpoll();
        static void   epollInit(ServerList &servers);
        static void   addStdinToEpoll();
        // utils
        static void   setEpollEvents(int fd, int option, uint32_t events);
        static void   setServerFromListener(Client &client);
        static void   setLocation(Client &state);

        // main loop
        static void   runServers();
        static void   handleEvents(size_t eventCount);
        static bool   handleEpollStdinEvents();
        static bool   handleEpollErrorEvents(const struct epoll_event &currentEvent, int eventFD);
        static void   acceptConnection(const int listener);
        static bool   addFdToEpoll(int infd);
        static void   setClientServerAddress(Client &client, int infd);
        static bool   runHandleTransfer(struct epoll_event &currentEvent);
        static bool   runCgiHandleTransfer(struct epoll_event &currentEvent);
        static void   processClientRequest(Client &client);
        static size_t receiveClientData(Client &client, char *buff);

        // cleanup
        static void   disconnectChecks();
        static void   checkCgiDisconnect();
        static void   checkClientDisconnects();
        static void   removeHandlesWithFD(int fd);
        static void   closeHandles(pid_t pid);
        static void   clientHttpCleanup(Client &client);
        static void   cleanupClient(Client &client);
        static void   cleanupHandleCgi(vector<unique_ptr<HandleTransfer>>::iterator it, pid_t pid);

        static inline void insertHandleTransfer(unique_ptr<HandleTransfer> handle)
        {
            _handle.push_back(move(handle));
        }

        static inline void insertHandleTransferCgi(unique_ptr<HandleTransfer> handle)
        {
            _handleCgi.push_back(move(handle));
        }

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
        static inline int getEpollFD()
        {
            return _epfd;
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
        // --- Server configuration ---
        static string _serverRootDir;
        static ServerList _servers;
        static vector<int> _listenFDS;

        // --- Epoll and event management ---
        static int _epfd;
        static array<struct epoll_event, FD_LIMIT> _events;

        // --- Client and handle management ---
        static unordered_map<int, unique_ptr<Client>> _clients;
        static vector<unique_ptr<HandleTransfer>> _handle;
        static vector<unique_ptr<HandleTransfer>> _handleCgi;

        // --- Miscellaneous ---
        static int _level;
        static uint64_t _clientBufferSize;
        static uint64_t _ramBufferLimit;
};
#endif
