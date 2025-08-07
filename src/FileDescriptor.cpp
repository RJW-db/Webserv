#include <FileDescriptor.hpp>
#include <RunServer.hpp>

// array<int, FD_LIMIT> FileDescriptor::_fds = {};
// vector<int> FileDescriptor::_fds(FD_LIMIT);
vector<int> FileDescriptor::_fds = {};
// map<chrono::steady_clock::time_point, int> FileDescriptor::_clientFDS = {};

void	FileDescriptor::initializeAtexit()
{
    atexit(cleanupFD);
}

void FileDescriptor::cleanupFD()
{
    for (int fd : _fds)
	{
        closeFD(fd);
	}
}


void	FileDescriptor::setFD(int fd)
{
	if (_fds.size() >= FD_LIMIT)
	{
		cerr << "File descriptor limit reached" << endl;
		return;
	}
	_fds.push_back(fd);
}

void	FileDescriptor::closeFD(int &fd)
{
    if (fd == -1)
        return;
    vector<int>::iterator it = find(_fds.begin(), _fds.end(), fd);
    if (it != _fds.end())
	{
        if (safeCloseFD(fd) == false)
        {
            // finishes up clients and restart webserv
            // throw something
        }
        _fds.erase(it);
        fd = -1;
    }
    else
    {
        cerr << "File descriptor " << fd << " not found in the vector." << endl;
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
        std::cerr << "No more new connenctions and finish up current connections and then restart webserver" << std::endl; //testcout
        return false;
    }
    return true;
}
