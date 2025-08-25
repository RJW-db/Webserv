#ifndef LOCATION_HPP
# define LOCATION_HPP

#include <Aconfig.hpp>
#include <string>

using namespace std;

class Alocation : public Aconfig
{
	public:
    //initialization
		Alocation &operator=(const Alocation &other);

        //getters
        uint8_t getAllowedMethods() const;
		string getUploadStore() const;
		vector<string> getExtension() const;
		string getCgiPath() const;
		string getPath() const;

        //helper function
        bool isCgiFile(string_view &filename) const;

	protected:
        //initialization for inheritance
    	Alocation() = default;
		Alocation(const Alocation &other);

        //values to be stored
        uint8_t _allowedMethods = 0;
		string _upload_store;
		vector<string> _cgiExtension;
		string _cgiPath;
		string _locationPath; // includes root + path;
};


class Location : public Alocation
{
	public:
        //initialization
		Location() = default;
		Location(string &path);
		Location(const Location &other);
		Location &operator=(const Location &other);
		~Location() = default;

        //getpath
		void getLocationPath(string &line);

        //parsing logic
		bool methods(string &line);
		bool indexPage(string &line);
		bool uploadStore(string &line);
		bool cgiExtensions(string &line);
		bool cgiPath(string &line);

        //default setting
		void SetDefaultLocation(Aconfig &curConf);


	private:
        //helper
		bool checkMethodEnd(bool &findColon, string &line);
};

#endif
