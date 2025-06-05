#ifndef FILEDESCRIPTOR
# define FILEDESCRIPTOR

// #include <RunServer.hpp>
#include <unistd.h>
#include <cstdint>
#include <iostream>	// runtime_error
// #include <stdexcept>	// runtime_error
// #include <array>
#include <vector>
#include <algorithm> // find()
		
class FileDescriptor
{
	public:
		FileDescriptor();
		~FileDescriptor();

		void	setFD(int fd);
		void	closeFD(int fd);
	private:
		static std::vector<int> _fds;
};


#endif