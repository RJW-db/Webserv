#ifndef LOCATION_HPP
# define LOCATION_HPP
#include <string>
#include "Aconfig.hpp"
using namespace std;

class Alocation : public Aconfig
{
	public:
    	// Initialization
		Alocation &operator=(const Alocation &other);

        // Getters
		uint8_t getAllowedMethods() const {
			return _allowedMethods;
		}
		const string &getUploadStore() const {
			return _upload_store;
		}
		const vector<string> &getExtension() const {
			return _cgiExtension;
		}
		const string &getCgiPath() const {
			return _cgiPath;
		}
		const string &getPath() const {
			return _locationPath;
		}

        // Helper function
        bool isCgiFile(string_view &filename) const;

	protected:
        // Initialization for inheritance
    	Alocation() = default;
		Alocation(const Alocation &other);

        // Values to be stored
        uint8_t _allowedMethods = 0;
		string _upload_store;
		vector<string> _cgiExtension;
		string _cgiPath;
		string _locationPath; // includes root + path;
};

class Location : public Alocation
{
	public:
        // Initialization
		Location() = default;
		Location(const Location &other);
		Location &operator=(const Location &other);
		// ~Location() = default;

        // Getters
		void getLocationPath(string &line);

        // Parsing logic
		bool methods(string &line);
		bool uploadStore(string &line);
		bool cgiExtensions(string &line);
		bool cgiPath(string &line);

        // Default setting
		void SetDefaultLocation(const Aconfig &curConf);


	private:
        // Helper
		bool checkMethodEnd(bool &findColon, string &line);
};
#endif
