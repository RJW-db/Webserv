#include <dirent.h>
#include <cstring>
#include <algorithm>
#include "utils.hpp"
#include "RunServer.hpp"
#include "Logger.hpp"


namespace
{
    void insertUuidSegment(int8_t amount, char *uuidIndex);
}

void initRandomSeed()
{
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    if (seed <= 0)
        throw runtime_error(string("Server epoll_wait: ") + strerror(errno));
    srand(static_cast<unsigned int>(seed));
}

bool directoryCheck(string &path)
{
    // std::cout << "\t" << path << std::endl;
    DIR *d = opendir(path.c_str());	// path = rde-brui
    if (d == NULL)
    {
        perror("opendir");
        return false;
    }

    // struct dirent *directoryEntry;
    // while ((directoryEntry = readdir(d)) != NULL) {
    //     printf("%s\n", directoryEntry->d_name);
    //     if (string(directoryEntry->d_name) == path)
    //     {
    //         closedir(d);
    //         return (true);
    //     }
    // }
    
    closedir(d);
    return (true);
    // return (false);
}

size_t getFileLength(const string_view filename)
{
    struct stat status;
    if (stat(filename.data(), &status) == -1)
        throw RunServers::ClientException("text");

    if (status.st_size < 0)
        throw RunServers::ClientException("Invalid file size");

    return static_cast<size_t>(status.st_size);;
}

uint64_t stoullSafe(string_view stringValue)
{
    if (stringValue.empty())
        throw runtime_error("Given value is \"\"");
    if (all_of(stringValue.begin(), stringValue.end(), ::isdigit) == false)
        throw runtime_error("Given value contains non-digit characters: " + string(stringValue));

    unsigned long long value;
    try
    {
        value = stoull(stringValue.data());
    }
    catch (const invalid_argument &)
    {
        throw runtime_error("Given value is invalid: " + string(stringValue));
    }
    catch (const out_of_range &)
    {
        throw runtime_error("Given value is out of range: " + string(stringValue));
    }
    return static_cast<uint64_t>(value);
}

string escapeSpecialChars(const string &input, bool useColors)
{
    const string RED = useColors ? "\033[31m" : "";
    const string MAGENTA = useColors ? "\033[35m" : "";
    const string RESET = useColors ? "\033[0m" : "";

    string result;
    for (char ch : input) {
        if (ch == '\r') {
            result += RED + "\\r" + RESET;
        } else if (ch == '\n') {
            result += MAGENTA + "\\n\n" + RESET;
        } else if (ch == '\t') {
            result += RED + "\\t" + RESET;
        } else {
            result += ch;
        }
    }
    return result;
}

void throwTesting()
{
    static uint8_t count = 1;

    // Logger::log(DEBUG, "ThrowTesting(), count: ", +count); //testlog
    if (count++ < 2)
    {
        throw bad_alloc();
        // throw runtime_error("Throw test");
    }
}

void generateUuid(char uuid[UUID_SIZE])
{
    uuid[0] = '-';
    insertUuidSegment(8, uuid + 1);   // uuid[1-8]   = random, uuid[9]  = '-'
    insertUuidSegment(4, uuid + 10);  // uuid[10-13] = random, uuid[14] = '-'  
    insertUuidSegment(4, uuid + 15);  // uuid[15-18] = random, uuid[19] = '-'
    insertUuidSegment(4, uuid + 20);  // uuid[20-23] = random, uuid[24] = '-'
    insertUuidSegment(12, uuid + 25); // uuid[25-36] = random, uuid[37] = '-'
    uuid[UUID_SIZE - 1] = '\0';
}

namespace
{
    inline void insertUuidSegment(int8_t amount, char *uuidIndex)
    {
        const char *hexCharacters = "0123456789abcdef";
        int8_t i;
        for (i = 0; i < amount; ++i) {
            uuidIndex[i] = hexCharacters[rand() % 16];
        }
        uuidIndex[i] = '-';
    }
}
