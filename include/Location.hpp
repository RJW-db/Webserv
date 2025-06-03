#ifndef LOCATION_HPP
# define LOCATION_HPP

// #include <RunServer.hpp>
#include <Aconfig.hpp>
#include <cstring>
#include <string>
#include <array>
using namespace std;

class Location : public Aconfig
{
	public :
		Location();
		Location(const Location &other);
		Location &operator=(const Location &other);
		~Location() = default;

		void setPath(string &line);

		bool methods(string &line);
		bool indexPage(string &line);
		bool uploadStore(string &line);
		bool extension(string &line);
		bool cgiPath(string &line);
		
		
		
		string _path;
		array<string, 3> _methods;
		string _upload_store;
		string _cgiExtension;
		string _cgiPath;
		
	private :
		bool checkMethodEnd(bool &findColon, string &line);
	};

#endif
