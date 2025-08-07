#include <utils.hpp>
#include <RunServer.hpp>

#include <filesystem> // canonical()
#include <dirent.h>

string getExecutableDirectory()
{
    try {
        return filesystem::canonical("/proc/self/exe").parent_path().string();
    } catch (const exception& e) {
        throw runtime_error("Cannot determine executable directory: " + string(e.what()));
    }
}

void initRandomSeed()
{
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    if (seed <= 0)
        throw runtime_error(string("Server epoll_wait: ") + strerror(errno));
    srand(static_cast<unsigned int>(seed));
}

void generateUuid(char uuid[UUID_SIZE])
{
    insertUuidSegment(8, uuid);       // uuid[0-7]   = random, uuid[8]  = '-'
    insertUuidSegment(4, uuid + 9);   // uuid[9-12]  = random, uuid[13] = '-'  
    insertUuidSegment(4, uuid + 14);  // uuid[14-17] = random, uuid[18] = '-'
    insertUuidSegment(4, uuid + 19);  // uuid[19-22] = random, uuid[23] = '-'
    insertUuidSegment(12, uuid + 24); // uuid[24-35] = random, uuid[36] = '-'
    uuid[UUID_SIZE - 1] = '\0';
}

static inline void insertUuidSegment(int8_t amount, char *uuidIndex)
{
    const char *hexCharacters = "0123456789abcdef";
    int8_t i;
    for (i = 0; i < amount; ++i) {
        uuidIndex[i] = hexCharacters[rand() % 16];
    }
    uuidIndex[i] = '-';
}

bool directoryCheck(string &path)
{
	// std::cout << "\t" << path << std::endl;
    DIR *d = opendir(path.c_str());	// path = rde-brui
    if (d == NULL) {
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
    {
        throw RunServers::ClientException("text");
    }

    if (status.st_size < 0)
        throw RunServers::ClientException("Invalid file size");

    return static_cast<size_t>(status.st_size);;
}

uint64_t stoullSafe(string_view stringValue)
{
    if (stringValue.empty())
    {
        throw runtime_error("Given buffer_size for Webserv is \"\"");
    }
    if (all_of(stringValue.begin(), stringValue.end(), ::isdigit) == false)
    {
        throw runtime_error("Given buffer_size for Webserv contains non-digit characters: " + string(stringValue));
    }
    unsigned long long value;
    try
    {
        value = stoull(stringValue.data());
    }
    catch (const invalid_argument &)
    {
        throw runtime_error("Given buffer_size for Webserv is invalid: " + string(stringValue));
    }
    catch (const out_of_range &)
    {
        throw runtime_error("Given buffer_size for Webserv is out of range: " + string(stringValue));
    }
    return static_cast<uint64_t>(value);
}

