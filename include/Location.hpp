#ifndef LOCATION_HPP
# define LOCATION_HPP

#include <Webserv.hpp>
#include <Aconfig.hpp>
#include <cstring>
#include <string>

using namespace std;

class Location : public Aconfig
{
	public :
		Location();
		Location(const Location &other);
		Location &operator=(const Location &other);
		~Location() = default;

		string setPath(string line);
		string methods(string line, bool &findColon);
		string indexPage(string line , bool &findColon);
		string uploadStore(string line , bool &findColon);

		string _path;
		array<string, 2> _methods;
		vector<string> _indexPage;
		string _upload_store;

	private :
};

#endif
