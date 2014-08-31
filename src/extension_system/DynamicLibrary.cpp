/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014
		Distributed under the Boost Software License, Version 1.0.
		(See accompanying file LICENSE_1_0.txt or copy at
		http://www.boost.org/LICENSE_1_0.txt)
*/
#include <extension_system/macros.hpp>
#include <extension_system/DynamicLibrary.hpp>

#include <stdexcept>
#include <sstream>

using namespace extension_system;

#ifdef EXTENSION_SYSTEM_OS_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#define NO_STRICT
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0601
	#endif
	#include <windows.h>
#else // posix e.g. linux
	#include <dlfcn.h>
#endif

const std::string DynamicLibrary::_file_extension =
#ifdef EXTENSION_SYSTEM_OS_WINDOWS
	".dll";
#else
	".so";
#endif

static void throwOnError() {
#ifdef EXTENSION_SYSTEM_OS_WINDOWS
	auto err = GetLastError();
#else
	auto err = dlerror();
#endif

	if(err) {
		std::stringstream sstream;
		sstream << "DynamicLibrary error: " << err;
		throw std::runtime_error(sstream.str());
	}
}

static void *loadLibrary(const std::string &filename)
{
	void *handle = nullptr;
#ifdef EXTENSION_SYSTEM_OS_WINDOWS
	handle = LoadLibraryA(filename.c_str());
#else
	handle = dlopen(filename.c_str(), RTLD_LAZY);
#endif
	throwOnError();
	return handle;
}

DynamicLibrary::DynamicLibrary(const std::string &filename)
	: _filename(filename), _handle(loadLibrary(filename)) {
}

DynamicLibrary::~DynamicLibrary() {
#ifdef EXTENSION_SYSTEM_OS_WINDOWS
	FreeLibrary(_handle);
#else
	dlclose(_handle);
#endif
}

std::string DynamicLibrary::getFilename() const {
	return _filename;
}

const void *DynamicLibrary::getHandle() const {
	return _handle;
}

void *DynamicLibrary::getProcAddress(const std::string &name)
{
	void *func;
#ifdef EXTENSION_SYSTEM_OS_WINDOWS
	static_assert(sizeof(void *) == sizeof(void (*)(void)), "object pointer and function pointer sizes must equal");
	auto tmp = GetProcAddress(_handle, name.c_str());
	func = *(void**)&tmp;
#else
	func = dlsym(_handle, name.c_str());
#endif
	throwOnError();
	return func;
}

std::string DynamicLibrary::fileExtension()
{
	return _file_extension;
}
