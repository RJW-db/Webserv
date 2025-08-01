#ifndef LOCATION_HPP
# define LOCATION_HPP

#include <utils.hpp>
#include <Aconfig.hpp>
#include <cstring>
#include <string>
#include <array> // required for std::array
using namespace std;

// class ConfigServer; // Forward declaration to avoid circular dependency
// class Aconfig;

class Alocation : public Aconfig
{
	public:
		Alocation &operator=(const Alocation &other);

        uint8_t getAllowedMethods() const;
		string getUploadStore() const;
		string getExtension() const;
		string getCgiPath() const;
		string getPath() const;

	protected:
    	Alocation() = default;
		Alocation(const Alocation &other);
        uint8_t _allowedMethods = 0;
		string _upload_store;
		string _cgiExtension;
		string _cgiPath;
		string _locationPath; // includes root + path;
};


class Location : public Alocation
{
	public:
		Location() = default;
		Location(string &path);
		Location(const Location &other);
		Location &operator=(const Location &other);
		~Location() = default;

		void getLocationPath(string &line);

		bool methods(string &line);
		bool indexPage(string &line);
		bool uploadStore(string &line);
		bool cgiExtension(string &line);
		bool cgiPath(string &line);

		void SetDefaultLocation(Aconfig &curConf);
		
		
	private:
		bool checkMethodEnd(bool &findColon, string &line);
};

#endif
