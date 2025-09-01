#ifndef UTILS_HPP
#define UTILS_HPP
#include <string>
#include <cstdlib>
using namespace std;
namespace
{
    static constexpr bool   TERMINAL = true;
    static constexpr bool   LOGGER = false;
    static constexpr size_t UUID_SIZE = 38;
}

void	 initRandomSeed();
bool	 directoryCheck(string &path);
size_t	 getFileLength(const string_view filename);
uint64_t stoullSafe(string_view stringValue);
string   escapeSpecialChars(const string &input, bool useColors);
void     throwTesting();
/**
 * Universally Unique Identifier
 * 3fb17ebc-bc38-4939-bc8b-74f2443281d4
 * 8 dash 4 dash 4 dash 4 dash 12
 */
void 	 generateUuid(char uuid[UUID_SIZE]);
#endif
