#ifndef SERVER_HPP
# define SERVER_HPP

#include <unistd.h>



class FileDescriptor
{
	public:
		FileDescriptor(int fd);
		~FileDescriptor();
	private:
		int _fd;
};

FileDescriptor::FileDescriptor(int fd) : _fd(fd)
{
}

FileDescriptor::~FileDescriptor()
{
	close(_fd);
}
#endif