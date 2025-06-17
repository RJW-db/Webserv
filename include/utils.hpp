#ifndef UTILS_HPP
# define UTILS_HPP

#include <string>
#include <sys/stat.h>
#include <memory>

using namespace std;

// class Server;
// class Location;




bool directoryCheck(string &path);
size_t getFileLength(const string_view filename);


#endif
