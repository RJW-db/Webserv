#include <FileDescriptor.hpp>

// std::array<int, FD_LIMIT> FileDescriptor::_fds = {};
// std::vector<int> FileDescriptor::_fds(FD_LIMIT);
std::vector<int> FileDescriptor::_fds = {};
FileDescriptor::FileDescriptor()
{
	_fds.reserve(FD_LIMIT);
}

FileDescriptor::~FileDescriptor()
{
	for (int fd : _fds)
	{
		if (fd != -1)
		{
			close(fd);
			std::cout << "Closing: " << fd << std::endl;
		}
	}
}

void	FileDescriptor::setFD(int fd)
{
	if (_fds.size() >= FD_LIMIT)
	{
		std::cerr << "File descriptor limit reached" << std::endl;
		return;
	}
	_fds.push_back(fd);
}

void	FileDescriptor::closeFD(int fd)
{
    std::vector<int>::iterator it = std::find(_fds.begin(), _fds.end(), fd);

    if (it != _fds.end())
	{
        close(fd);
        _fds.erase(it);
    }
    else
    {
        std::cerr << "File descriptor " << fd << " not found in the vector." << std::endl;
    }
}
