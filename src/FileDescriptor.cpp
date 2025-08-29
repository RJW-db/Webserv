#ifdef __linux__
# include <sys/epoll.h> // EPOLL_CTL_DEL
#endif
#include <fcntl.h>
#include "FileDescriptor.hpp"
#include "RunServer.hpp"
#include "Logger.hpp"

// array<int, FD_LIMIT> FileDescriptor::_fds = {};
// vector<int> FileDescriptor::_fds(FD_LIMIT);
vector<int> FileDescriptor::_fds = {};
// map<chrono::steady_clock::time_point, int> FileDescriptor::_clientFDS = {};

void FileDescriptor::cleanupEpollFd(int &fd)
{
    if (fd > 0)
    {
        vector<int> &fds = RunServers::getEpollAddedFds();
        auto it = find(fds.begin(), fds.end(), fd);
        if (it != fds.end())
        {
            RunServers::setEpollEvents(fd, EPOLL_CTL_DEL, EPOLL_DEL_EVENTS);
            fds.erase(it);
        }
        FileDescriptor::closeFD(fd);
    }
}

void FileDescriptor::cleanupAllFD()
{
    while (!_fds.empty())
    {
        int fd = _fds.back();
        closeFD(fd);
    }
}

void FileDescriptor::printAllFDs()
{
    Logger::log(DEBUG, "Current tracked FDs (", _fds.size(), " total):");
    for (size_t i = 0; i < _fds.size(); i++) {
        Logger::log(DEBUG, "  [", i, "] FD: ", _fds[i]);
    }
    if (_fds.empty()) {
        Logger::log(DEBUG, "  No FDs currently tracked");
    }
}

bool	FileDescriptor::closeFD(int &fd)
{
    if (fd == -1)
        return true;
    vector<int>::iterator it = find(_fds.begin(), _fds.end(), fd);
    if (it != _fds.end())
    {
        // std::cerr << "closing fd " << fd << std::endl; //testcout
        if (safeCloseFD(fd) == false) // TODO EIO, how to resove for parent and child
            return false;
        _fds.erase(it);
        Logger::log(CHILD_INFO, "Closed file descriptor:", fd);
        fd = -1;
    }
    else
    {
        if (fd != -1)
            Logger::log(WARN, "FileDescriptor error", fd, "Not in vector", "closeFD attempted");
    }
    return true;
}

// bool only used for child with return false
bool    FileDescriptor::safeCloseFD(int fd)
{
    int ret;
    do
    {
        ret = close(fd);
    } while (ret == -1 && errno == EINTR);

    if (ret == -1 && errno == EIO)
    {
        Logger::log(FATAL, "FileDescriptor::safeCloseFD: Attempted to close a file descriptor that is not in the vector: ", fd);
        RunServers::fatalErrorShutdown();
        return false;
    }
    return true;
}

bool FileDescriptor::setNonBlocking(int sfd)
{
    int currentFlags = fcntl(sfd, F_GETFL, 0);
    if (currentFlags == -1)
    {
        Logger::log(ERROR, "Server error", sfd, "fcntl F_GETFL failed", strerror(errno));
        return false;
    }

    currentFlags |= O_NONBLOCK;
    int fcntlResult = fcntl(sfd, F_SETFL, currentFlags);
    if (fcntlResult == -1)
    {
        Logger::log(ERROR, "Server error", sfd, "fcntl F_SETFL failed", strerror(errno));
        return false;
    }
    return true;
}
