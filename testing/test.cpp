#include <string>
#include <string_view>
#include <iostream>

// int main()
// {
// 	std::string str = "Hello, World!";
// 	std::string_view view = str.substr(0, 5); // Extract "Hello"
// 	std::cout << view << std::endl;
// 	str.clear(); // Clear the original string
// 	std::cout << view << std::endl; // Still valid, prints "Hello"
// 	return 0;
// }

int main()
{
	std::string str = "Hello, World!";
	std::string_view view(str.data(), 5);
	std::cout << view << std::endl;
	str.clear(); // Clear the original string
	std::cout << view << std::endl; // view is freed, reading from it is undefined behavior
	return 0;
}