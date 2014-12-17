/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014
		Distributed under the Boost Software License, Version 1.0.
		(See accompanying file LICENSE_1_0.txt or copy at
		http://www.boost.org/LICENSE_1_0.txt)
*/
#include <extension_system/filesystem.hpp>

#ifndef EXTENSION_SYSTEM_USE_STD_FILESYSTEM

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

bool extension_system::filesystem::exists(const extension_system::filesystem::path &p)
{
	const std::string str = p.string();
	return access(str.c_str(), R_OK) == 0;
}


bool extension_system::filesystem::is_directory(const extension_system::filesystem::path &p)
{
	const std::string str = p.string();
	struct stat sb;
	return (stat(str.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}


extension_system::filesystem::path extension_system::filesystem::canonical(const extension_system::filesystem::path &p)
{
#ifdef EXTENSION_SYSTEM_COMPILER_MINGW
	return p;
#else
	char buffer[PATH_MAX];
	const std::string path = p.string();
	const char *result = realpath(path.c_str(), buffer);
	if(result == nullptr)
		return p; // couldn't resolve symbolic link
	else
		return std::string(result);
#endif
}

void extension_system::filesystem::forEachFileInDirectory(const extension_system::filesystem::path &p, const std::function<void(const extension_system::filesystem::path &p)> &func)
{
	struct dirent *ep = nullptr;
	const std::string path = p.string();
	DIR *dp = opendir (path.c_str());

	if (dp != nullptr)
	{
		while ((ep = readdir (dp))) {
			const std::string name = ep->d_name;
			if(name!="." && name!="..")
				func(path + "/" + name);
		}

		(void) closedir (dp);
	}
	//else throw std::runtime_error("Couldn't open the directory");
}

#endif
