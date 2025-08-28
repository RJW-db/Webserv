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
        static void cleanupFD(int &fd);
        static void cleanupAllFD();

		static void	setFD(int fd);
		static bool	closeFD(int &fd);
		static bool safeCloseFD(int fd);
		static void addClientFD(int clientFD);
		static void removeClientFD(int clientFD);
		static bool containsClient(int ClientFD);
		static void keepAliveCheck();
        static bool setNonBlocking(int sfd);
		
		static void printAllFDs();

	private:
		static vector<int> _fds;
        FileDescriptor() = delete;
        // static std::vector<int>& getFds();
};


#endif