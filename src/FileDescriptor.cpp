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
		if (fd != -1)
		{
			close(fd);
		}
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

void	FileDescriptor::closeFD(int fd)
{
    vector<int>::iterator it = find(_fds.begin(), _fds.end(), fd);
    if (it != _fds.end())
	{
        close(fd);
        _fds.erase(it);
    }
    else
    {
        cerr << "File descriptor " << fd << " not found in the vector." << endl;
    }
}
