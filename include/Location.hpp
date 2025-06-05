#ifndef LOCATION_HPP
# define LOCATION_HPP

#include <utils.hpp>
#include <Aconfig.hpp>
#include <cstring>
#include <string>
#include <array> // required for std::array
using namespace std;

// class ConfigServer; // Forward declaration to avoid circular dependency

class Location : public Aconfig
{
	public:
		Location();
		Location(const Location &other);
		Location &operator=(const Location &other);
		~Location() = default;

		string getLocationPath(string &line);

		bool methods(string &line);
		bool indexPage(string &line);
		bool uploadStore(string &line);
		bool extension(string &line);
		bool cgiPath(string &line);

		// string getPath() const;
		array<string, 3> getMethods() const;
		string getUploadStore() const;
		string getExtension() const;
		string getCgiPath() const;

		void SetDefaultLocation(Aconfig &curConf);
		
		
	private:
		bool checkMethodEnd(bool &findColon, string &line);

		// string _path;
		array<string, 3> _methods;
		string _upload_store;
		string _cgiExtension;
		string _cgiPath;
		
	private:
		bool checkMethodEnd(bool &findColon, string &line);
};

#endif
