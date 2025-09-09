#include <sys/stat.h>
#include <algorithm>
#include <dirent.h>
#include <cstring>
#include "RunServer.hpp"
#include "Logger.hpp"
#include "utils.hpp"
#include "ErrorCodeClientException.hpp"
namespace
{
    constexpr char hexCharacters[] = "0123456789abcdef";
    constexpr int  hexCharactersSize = sizeof(hexCharacters) - 1;

    void insertUuidSegment(int8_t amount, char *uuidIndex);
}

void initRandomSeed()
{
    auto seed = chrono::high_resolution_clock::now().time_since_epoch().count();
    if (seed <= 0)
        throw runtime_error(string("Server epoll_wait: ") + strerror(errno));
    srand(static_cast<unsigned int>(seed));
}

vector<string> listFilesInDirectory(Client &client, const string &path)
{
    DIR *d = opendir(path.c_str());
    if (d == NULL)
        throw ErrorCodeClientException(client, 500, "Couldn't open directory: " + path + " because: " + strerror(errno));

    /**
     * If an error occurs, NULL is returned and errno is set appropriately.
     * To distinguish end of stream from an error,
     * set errno to zero before calling readdir() and check its value when NULL is returned.
     */
    errno = 0;
    vector<string> files = {};
    struct dirent *directoryEntry;
    while ((directoryEntry = readdir(d)) != NULL) {
        char *file = directoryEntry->d_name;
        if (!(file[0] == '.' && (file[1] == '\0' || (file[1] == '.' && file[2] == '\0'))))
            files.push_back(file);
    }
    closedir(d);
    if (errno != 0)
        throw ErrorCodeClientException(client, 500, "Error reading directory: " + path + " because: " + strerror(errno));
    return files;
}

size_t getFileLength(Client &client, const string_view filename)
{
    struct stat status;
    if (stat(filename.data(), &status) == -1)
        throw ErrorCodeClientException(client, 400, "Filename: " + string(filename) + "Couldn't get info because" + strerror(errno));

    if (status.st_size < 0)
        throw ErrorCodeClientException(client, 400, "Filename: " + string(filename) + "invalid file size" + strerror(errno));
    return static_cast<size_t>(status.st_size);;
}

uint64_t stoullSafe(string_view stringValue)
{
    if (stringValue.empty())
        throw runtime_error("Given value is \"\"");
    if (all_of(stringValue.begin(), stringValue.end(), ::isdigit) == false)
        throw runtime_error("Given value contains non-digit characters: " + string(stringValue));

    unsigned long long value;
    try {
        value = stoull(stringValue.data());
    }
    catch (const invalid_argument &) {
        throw runtime_error("Given value is invalid: " + string(stringValue));
    }
    catch (const out_of_range &) {
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
        if (ch == '\r')
            result += RED + "\\r" + RESET;
        else if (ch == '\n')
            result += MAGENTA + "\\n\n" + RESET;
        else if (ch == '\t')
            result += RED + "\\t" + RESET;
        else
            result += ch;
    }
    return result;
}

// void throwTesting()
// {
//     static uint8_t count = 1;

//     // Logger::log(DEBUG, "ThrowTesting(), count: ", +count); //testlog
//     if (count++ == 1) {
//         Logger::logExit(ERROR, "Server Error", "Invalid FD, setEpollEvents failed");
//         // throw runtime_error("Throw test");
//     }
// }

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
        int8_t i;
        for (i = 0; i < amount; ++i)
            uuidIndex[i] = hexCharacters[rand() % hexCharactersSize];
        uuidIndex[i] = '-';
    }
}
