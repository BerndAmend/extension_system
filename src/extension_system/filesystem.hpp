/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014-2017
		Distributed under the Boost Software License, Version 1.0.
		(See accompanying file LICENSE_1_0.txt or copy at
		http://www.boost.org/LICENSE_1_0.txt)
*/
#pragma once

#include <extension_system/macros.hpp>

#include <string>
#include <vector>
#include <functional>

#include <extension_system/string.hpp>

#ifdef EXTENSION_SYSTEM_USE_STD_FILESYSTEM
	#include <filesystem>

	namespace extension_system { namespace filesystem {
		using namespace std::tr2::sys; // >=VS2012

		path canonical(const path &p);
		void forEachFileInDirectory(const path &p, const std::function<void(const path &p)> &func, bool recursive);
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
				path result;
				split(_pathname, "/", [&](const std::string &str) {
					result = str;
					return true;
				});
				return result;
			}

			path extension() const {
				const auto name = filename().string();
				if(name == "." || name == "..")
					return path();
				const auto pos = name.find_last_of('.');
				if(pos == std::string::npos)
					return path();
				return name.substr(pos);
			}

			 path operator/(const std::string& rhs) const {
				 return path(this->string() + "/" + rhs);
			 }

			 path operator/(const path& rhs) const {
				 return (*this) / rhs.string();
			 }

		private:
			std::string _pathname;
		};

		bool exists(const path &p);
		bool is_directory(const path &p);
		path canonical(const path &p);

		void forEachFileInDirectory(const path &p, const std::function<void(const filesystem::path &p)> &func, bool recursive);

	}}
#endif
