#include <string>
#include <string_view>
#include <iostream>

using namespace std;
// int main()
// {
// 	std::string str = "Hello, World!";
// 	std::string_view view = str.substr(0, 5); // Extract "Hello"
// 	std::cout << view << std::endl;
// 	str.clear(); // Clear the original string
// 	std::cout << view << std::endl; // Still valid, prints "Hello"
// 	return 0;
// }

// int main()
// {
// 	std::string str = "Hello, World!";
// 	std::string_view view(str.data(), 5);
// 	std::cout << view << std::endl;
// 	str.clear(); // Clear the original string
// 	std::cout << view << std::endl; // view is freed, reading from it is undefined behavior
// 	return 0;
// }

#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// int main(int argc, char *argv[])
// {
//     int pipefd[2];
//     pid_t cpid;
//     char buf;

//     if (argc != 2) {
// 		fprintf(stderr, "Usage: %s <string>\n", argv[0]);
// 		exit(EXIT_FAILURE);
//     }
//     if (pipe(pipefd) == -1) {
//         perror("pipe");
//         exit(EXIT_FAILURE);
//     }
//     cpid = fork();
//     if (cpid == -1) {
//         perror("fork");
//         exit(EXIT_FAILURE);
//     }
//     if (cpid == 0) {    /* Child reads from pipe */
//         close(pipefd[1]);          /* Close unused write end */
//         while (read(pipefd[0], &buf, 1) > 0)
//             write(STDOUT_FILENO, &buf, 1);
//         write(STDOUT_FILENO, "\n", 1);
//         close(pipefd[0]);
//         _exit(EXIT_SUCCESS);
//     } else {            /* Parent writes argv[1] to pipe */
//         close(pipefd[0]);          /* Close unused read end */
//         write(pipefd[1], argv[1], strlen(argv[1]));
//         close(pipefd[1]);          /* Reader will see EOF */
//         wait(NULL);                /* Wait for child */
//         exit(EXIT_SUCCESS);
//     }
// }


#include <chrono>
#include <cstdlib>
static constexpr size_t UUID_SIZE = 38;

// Static Function
static inline void insertUuidSegment(int8_t amount, char *buffIndex);

/**
 * Universally Unique Identifier
 * 3fb17ebc-bc38-4939-bc8b-74f2443281d4
 * 8 dash 4 dash 4 dash 4 dash 12
 */
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

static inline void insertUuidSegment(int8_t amount, char *buffIndex)
{
    const char *hexCharacters = "0123456789abcdef";
    int8_t i;
    for (i = 0; i < amount; ++i) {
        buffIndex[i] = hexCharacters[rand() % 16];
    }
    buffIndex[i] = '-';
}

void initRandomSeed()
{
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    if (seed <= 0)
    srand(static_cast<unsigned int>(seed));
}

#include <filesystem> // canonical()
// #include <stdexcept>

string getExecutableDirectory()
{
    try {
        return filesystem::canonical("/proc/self/exe").parent_path().string();
    } catch (const exception& e) {
        throw runtime_error("Cannot determine executable directory: " + string(e.what()));
    }
}

int main(void)
{
    char uuid[UUID_SIZE];
    generateUuid(uuid);
    printf("%s\n", uuid);

    std::cout << getExecutableDirectory() << std::endl; //testcout
    return 0;
}