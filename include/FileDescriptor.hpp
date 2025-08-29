#ifndef FILEDESCRIPTOR
#define FILEDESCRIPTOR
#include <algorithm>
#include <unistd.h>
#include <iostream>
#include <cstdint>
#include <chrono>
#include <vector>
#include <map>
using namespace std;

class FileDescriptor
{
	public:
		static void cleanupEpollFd(int &fd);
		static void cleanupAllFD();

		static inline void setFD(int fd) {
			_fds.push_back(fd);
		}
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
};


#endif