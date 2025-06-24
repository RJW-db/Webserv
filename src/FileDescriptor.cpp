#include <FileDescriptor.hpp>
#include <RunServer.hpp>

// array<int, FD_LIMIT> FileDescriptor::_fds = {};
// vector<int> FileDescriptor::_fds(FD_LIMIT);
vector<int> FileDescriptor::_fds = {};
map<chrono::steady_clock::time_point, int> FileDescriptor::_clientFDS = {};
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
			cout << "Closing: " << fd << endl;
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
    std::cout << "closing fd:" << fd << std::endl;
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

void FileDescriptor::addClientFD(int clientFD)
{
	chrono::steady_clock::time_point timePoint = chrono::steady_clock::now() + chrono::seconds(20);
	for (pair<chrono::steady_clock::time_point, int> it : _clientFDS)
	{
		if (it.second == clientFD)
		{
			std::cout << "added: " << clientFD << std::endl;
			it.first = timePoint;
			return ;
		}
	}
	std::cout << "add:" << clientFD << std::endl;
	_clientFDS.insert({timePoint, clientFD});
}

void FileDescriptor::removeClientFD(int clientFD)
{
	for (auto it = _clientFDS.begin(); it != _clientFDS.end(); ++it)
	{
		if (it->second == clientFD)
		{
			_clientFDS.erase(it);
			return ;
		}
	}
}

bool FileDescriptor::containsClient(int clientFD)
{
	for (auto it = _clientFDS.begin(); it != _clientFDS.end(); ++it)
	{
		if (it->second == clientFD)
		{
			return true;
		}
	}
	return false;
}

void FileDescriptor::keepAliveCheck()
{
	chrono::steady_clock::time_point now = chrono::steady_clock::now();
	for (auto it = _clientFDS.begin(); it != _clientFDS.end();)
	{
		if (it->first <= now)
		{
			std::cout << "removed: " << it->second << std::endl;
			// RunServers::cleanupClient(RunServers::getClients()[it->second]);
			// RunServers::cleanupClient(it->second);
			it = _clientFDS.erase(it); // This iterates
		}
		else
			break ;
	}
}
