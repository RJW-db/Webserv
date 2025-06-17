#ifndef UTILS_HPP
# define UTILS_HPP

#include <string>
#include <sys/stat.h>
#include <memory>

using namespace std;

// class Server;
// class Location;


struct ClientRequestState
{
    bool headerParsed = false;
    string header;
    string body;
    string path;
    string method;
    size_t contentLength = 0;
    // unique_ptr<Server> usedServer;
    // Location UsedLocation;
};

bool directoryCheck(string &path);
size_t getFileLength(const string_view filename);


#endif
