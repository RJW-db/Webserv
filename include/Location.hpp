#ifndef LOCATION_HPP
# define LOCATION_HPP

#include <Webserv.hpp>
#include <Aconfig.hpp>
#include <string>

using namespace std;

class Location : public Aconfig
{
	public :
		Location();
		Location(const Location &other);
		Location operator=(const Location &other);
		~Location() = default;

		string setPath(string line);
		string methods(string line, bool &findColon);

	private :
		string _path;
		array<string, 3> _methods;
};

#endif
