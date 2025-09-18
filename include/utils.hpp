#ifndef UTILS_HPP
#define UTILS_HPP
#include <algorithm>
#include <cstdlib>
#include <string>
using namespace std;
class Client;
namespace
{
    static constexpr bool   TERMINAL = true;
    static constexpr bool   LOGGER = false;
    static constexpr size_t ID_SIZE = 32;
    static constexpr size_t UUID_SIZE = 38;
}

void	 initRandomSeed();
vector<string> listFilesInDirectory(Client &client, const string &path);
size_t	 getFileLength(Client &client, const string_view filename);
uint64_t stoullSafe(string_view stringValue);
string   escapeSpecialChars(const string &input, bool useColors);
char *generateSessionIdCookie(char sessionId[ID_SIZE]);
/**
 * Universally Unique Identifier
 * 3fb17ebc-bc38-4939-bc8b-74f2443281d4
 * 8 dash 4 dash 4 dash 4 dash 12
 */
char *generateFilenameUuid(char uuid[UUID_SIZE]);


inline void convertStringToLower(string &str) {
    transform(str.begin(), str.end(), str.begin(), ::tolower);
}
// void     throwTesting();
#endif
