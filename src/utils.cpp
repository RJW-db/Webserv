#include <utils.hpp>

#include <dirent.h>

bool directoryCheck(string &path)
{
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