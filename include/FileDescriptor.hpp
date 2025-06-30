#ifndef FILEDESCRIPTOR
# define FILEDESCRIPTOR

// #include <RunServer.hpp>
#include <unistd.h>
#include <cstdint>
#include <iostream>	// runtime_error
// #include <stdexcept>	// runtime_error
// #include <array>
#include <chrono>
#include <vector>
#include <algorithm> // find()
#include <map>

using namespace std;

class FileDescriptor
{
	public:
        static void initializeAtexit();
        static void cleanupFD();

		static void	setFD(int fd);
		static void	closeFD(int fd);
		static void addClientFD(int clientFD);
		static void removeClientFD(int clientFD);
		static bool containsClient(int ClientFD);
		static void keepAliveCheck();
		
	private:
		static vector<int> _fds;
        FileDescriptor() = delete;
        // static std::vector<int>& getFds();
};


#endif