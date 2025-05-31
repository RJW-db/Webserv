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
		bool uploadStore(string &line);
		bool extension(string &line);
		bool cgiPath(string &line);

		string _path;
		array<string, 3> _methods;
		string _upload_store;
		string _cgiExtension;
		string _cgiPath;

	private :
};

#endif
