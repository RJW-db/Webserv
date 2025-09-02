#include <algorithm>
#include <fcntl.h>
#ifdef __linux__
# include <sys/epoll.h> // EPOLL_CTL_DEL
#endif
#include "FileDescriptor.hpp"
#include "RunServer.hpp"
#include "Logger.hpp"
vector<int> FileDescriptor::_fds = {};
namespace
{
    constexpr uint32_t EPOLL_DEL_EVENTS = 0;
}

void FileDescriptor::setFD(int fd)
{
    try
    {
        _fds.push_back(fd);
    }
    catch (const bad_alloc& e)
    {
        safeCloseFD(fd);
        throw;
    }
    catch (...)
    {
        safeCloseFD(fd);
        throw;
    }
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

bool	FileDescriptor::closeFD(int &fd)
{
    if (fd == -1)
        return true;
    vector<int>::iterator it = find(_fds.begin(), _fds.end(), fd);
    if (it != _fds.end())
    {
        if (fd == Logger::getLogfd())
            Logger::log(CHILD_INFO, "Closed file descriptor:", fd);
        safeCloseFD(fd);
        _fds.erase(it);
        if (fd != Logger::getLogfd())
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

void FileDescriptor::cleanupAllFD()
{
    while (!_fds.empty())
        closeFD(_fds.back());
}

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

void FileDescriptor::printAllFDs()
{
    for (size_t i = 0; i < _fds.size(); i++) {
        Logger::log(DEBUG, "[", i, "] FD: ", _fds[i]);
    }
    if (_fds.empty()) {
        Logger::log(DEBUG, "No FDs currently tracked");
    }
}
