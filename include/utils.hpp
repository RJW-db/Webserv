#ifndef UTILS_HPP
# define UTILS_HPP

#include <string>
#include <sys/stat.h>
#include <memory>

using namespace std;

bool directoryCheck(string &path);
size_t getFileLength(const string_view filename);
uint64_t stoullSafe(string_view stringValue);

#endif
