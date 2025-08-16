#include <FileDescriptor.hpp>
#include <RunServer.hpp>
#include <Logger.hpp>

// array<int, FD_LIMIT> FileDescriptor::_fds = {};
// vector<int> FileDescriptor::_fds(FD_LIMIT);
vector<int> FileDescriptor::_fds = {};
// map<chrono::steady_clock::time_point, int> FileDescriptor::_clientFDS = {};

void FileDescriptor::cleanupFD()
{
    while (!_fds.empty())
    {
        int fd = _fds.back();
        closeFD(fd);
    }
}


void	FileDescriptor::setFD(int fd)
{
	// if (_fds.size() >= FD_LIMIT)
	// {
	// 	cerr << "File descriptor limit reached" << endl;
	// 	return;
	// }
	_fds.push_back(fd);
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

void	FileDescriptor::closeFD(int &fd)
{
    vector<int>::iterator it = find(_fds.begin(), _fds.end(), fd);
    if (it != _fds.end())
	{
        // std::cerr << "closing fd " << fd << std::endl; //testcout
        if (fd != -1 && safeCloseFD(fd) == false)
        {
            // finishes up clients and restart webserv
            // throw something
        }
        _fds.erase(it);
        fd = -1;
    }
    else
    {
        Logger::log(WARN, "FileDescriptor::closeFD: Attempted to close a file descriptor that is not in the vector: ", fd);
    }
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
        return false;
    }
    return true;
}
