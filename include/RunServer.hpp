#ifndef WEBSERV_HPP
# define WEBSERV_HPP
#include <signal.h>
#include <memory>
#include "HandleTransfer.hpp"
#include "ConfigServer.hpp"
#define _XOPEN_SOURCE 700  // VSC related, make signal and struct visible
#ifndef FD_LIMIT
# define FD_LIMIT 1024
#endif
using namespace std;
using ServerList = vector<unique_ptr<AconfigServ>>;
using HandleTransferIter = vector<unique_ptr<HandleTransfer>>::iterator;
extern volatile sig_atomic_t g_signal_status;
class Client;

class RunServers
{
    public:
        // initialization
        static void   getExecutableDirectory();
        static void   createServers(vector<ConfigServer> &configs);
        static void   setupEpoll();
        static void   epollInit(ServerList &servers);
        static void   addStdinToEpoll();
        static void   cleanupEpoll();
        
        // utils
        static void   setEpollEvents(int fd, int option, uint32_t events);
        static void   setEpollEventsClient(Client &client, int fd, int option, uint32_t events);
        static void   setServerFromListener(Client &client);
        static void   setLocation(Client &state);
        static inline void   fatalErrorShutdown() {
            _fatalErrorOccurred = true;
        };

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
        static HandleTransferIter cleanupHandleCgi(HandleTransferIter it, int clientFD);
        static HandleTransferIter killCgiPipes(HandleTransferIter it, pid_t pid);

        static inline void insertHandleTransfer(unique_ptr<HandleTransfer> handle) {
            _handle.push_back(move(handle));
        }
        static inline void insertHandleTransferCgi(unique_ptr<HandleTransfer> handle) {
            _handleCgi.push_back(move(handle));
        }
        static inline string &getServerRootDir() {
            return _serverRootDir;
        }
        static inline uint64_t getRamBufferLimit() {
            return _ramBufferLimit;
        }
        static inline int getEpollFD() {
            return _epfd;
        }
        static inline vector<int> &getEpollAddedFds() {
            return _epollAddedFds;
        }
        static inline vector<unique_ptr<HandleTransfer>> &getHandleTransfers() {
            return _handle;
        }

    private:
        // --- Server configuration ---
        static string _serverRootDir;
        static ServerList _servers;
        static vector<int> _listenFDS;
        static vector<int> _epollAddedFds;

        // --- Epoll and event management ---
        static int _epfd;
        static array<struct epoll_event, FD_LIMIT> _events;

        // --- Client and handle management ---
        static unordered_map<int, unique_ptr<Client>> _clients;
        static vector<unique_ptr<HandleTransfer>> _handle;
        static vector<unique_ptr<HandleTransfer>> _handleCgi;

        // --- Miscellaneous ---
        static int _level;
        static uint64_t _ramBufferLimit;
        static bool _fatalErrorOccurred;
};
#endif
