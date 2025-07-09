#include <utils.hpp>
#include <RunServer.hpp>

#include <dirent.h>

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
