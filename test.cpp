#include <iostream>
#include <iomanip>
#include <sstream>

#include <algorithm>
#include <string>
// void validateChunkSizeLine(const std::string& input)
// {
//     if (input.size() < 3)
//     {
//         std::cout << "wrong1" << std::endl; //testcout
//         // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
//     }

//     if (std::all_of(input.begin(), input.end() - 2, ::isxdigit) == false)
//     {
//         std::cout << "wrong2" << std::endl; //testcout
//         // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
//     }

//     size_t hexPart = input.size() - 2;
//     if (input[hexPart] != '\r' && input[hexPart + 1] != '\n')
//     {
//         std::cout << "wrong3" << std::endl; //testcout
//         if (input[hexPart] == '\r')
//             std::cout << "\\r" << std::endl; //testcout
//         if (input[hexPart + 1] == '\n')
//             std::cout << "\\n" << std::endl; //testcout
//         // throw ErrorCodeClientException(client, 400, "Invalid chunk size given: " + input);
//     }
// }
// #include <string>
// uint64_t parseChunkSize(const std::string& input)
// {
//     uint64_t chunkSize;
//     std::stringstream ss;
//     ss << std::hex << input;
//     ss >> chunkSize;
//     // std::cout << "chunkSize " << chunkSize << std::endl;
//     // if (static_cast<size_t>(chunkSize) > client._location.getClientBodySize())
//     // {
//     //     throw ErrorCodeClientException(client, 413, "Content-Length exceeds maximum allowed: " + to_string(chunkSize)); // (413, "Payload Too Large");
//     // }
//     return chunkSize;
// }

// void ParseChunkStr(const std::string& input, uint64_t chunkSize)
// {
//     if (input.size() - 2 != chunkSize)
//     {
//         std::cout << "test1" << std::endl; //testcout
//         // throw ErrorCodeClientException(client, 400, "Chunk data does not match declared chunk size");
//     }

//     if (input[chunkSize] != '\r' || input[chunkSize + 1] != '\n')
//     {
//         std::cout << "test2" << std::endl; //testcout
//         // throw ErrorCodeClientException(client, 400, "chunk data missing CRLF");
//     }
// }

// int main()
// {
//     std::string hexa = "0123456789ABCDEF";
    
//     std::string hexLen;
//     hexLen = "E\r\n";
//     std::string hexStr;
//     hexStr = "Good mornning!\r\n";

//     validateChunkSizeLine(hexLen);
//     uint64_t chunkSize = parseChunkSize(hexLen);
//     ParseChunkStr(hexStr, chunkSize);
//     std::cout << "success" << std::endl; //testcout
//     return 0;
// }

int main()
{
    std::string test = "";

    test.append("worked");
    std::cout << test << std::endl; //testcout
    return (0);
}