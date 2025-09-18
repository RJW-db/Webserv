#ifndef FILEDESCRIPTOR
#define FILEDESCRIPTOR
#include <vector>
using namespace std;

class FileDescriptor
{
	public:
		static void setFD(int &fd);
		static bool safeCloseFD(int fd);
		static bool	closeFD(int &fd);
		static bool setNonBlocking(int sfd);
		static void cleanupAllFD();
		static void cleanupEpollFd(int &fd);
		// static void printAllFDs();

	private:
		static vector<int> _fds;
		FileDescriptor() = delete;
};
#endif
