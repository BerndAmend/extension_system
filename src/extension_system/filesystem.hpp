/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014
		Distributed under the Boost Software License, Version 1.0.
		(See accompanying file LICENSE_1_0.txt or copy at
		http://www.boost.org/LICENSE_1_0.txt)
*/
#pragma once

#include <extension_system/macros.hpp>

#include <string>
#include <vector>

#include <extension_system/string.hpp>

#ifdef EXTENSION_SYSTEM_USE_STD_FILESYSTEM
	#include <filesystem>

	namespace extension_system { namespace filesystem {
		using namespace std::tr2::sys; // >=VS2012

		//
		path canonical(const path &p) {
			return filen.filename();
		}

		//
		std::vector<path> getDirectoryContent(const path &p) {
			std::vector<path> result;

			path someDir(path);
			directory_iterator end_iter;

			if ( exists(someDir) && is_directory(someDir))
				for( directory_iterator dir_iter(someDir); dir_iter != end_iter ; ++dir_iter)
					result.push_back(dir_iter>path());

			return result;
		}
	}}
#else
	namespace extension_system { namespace filesystem {
		class path
		{
		public:
			path() {}
			path(const path& p) { *this = p._pathname; }
			path(const std::string& p) { *this = p; }

			path &operator=(const path& p) {
				*this = p._pathname;
				return *this;
			}
			path &operator=(const std::string& p) {
				_pathname = p;
				return *this;
			}

			const std::string string() const { return _pathname; }
			const std::string generic_string() const { return _pathname; }

			path filename() const {
				auto splitted = split(_pathname, '/');
				if(splitted.empty())
					return path();
				else
					return path(splitted.back());
			}

			path extension() const {
				auto name = filename().string();
				if(name == "." || name == "..")
					return path();
				auto pos = name.find_last_of('.');
				if(pos == std::string::npos)
					return path();
				return name.substr(pos);
			}
		private:
			std::string _pathname;
		};

		bool exists(const path &p);
		bool is_directory(const path &p);
		path canonical(const path &p);

		std::vector<path> getDirectoryContent(const path &p);

	}}
#endif
