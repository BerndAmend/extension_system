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

DynamicLibrary::DynamicLibrary(const std::string &filename)
	: _filename(filename), _handle(nullptr) {
#ifdef EXTENSION_SYSTEM_OS_WINDOWS
	_handle = LoadLibraryA(filename.c_str());
#else
	_handle = dlopen(filename.c_str(), RTLD_LAZY);
#endif
	if(_handle == nullptr)
		setLastError();
}

DynamicLibrary::~DynamicLibrary() {
	if(isValid()) {
#ifdef EXTENSION_SYSTEM_OS_WINDOWS
		FreeLibrary(_handle);
#else
		dlclose(_handle);
#endif
	}
}

std::string DynamicLibrary::getFilename() const {
	return _filename;
}

const void *DynamicLibrary::getHandle() const {
	return _handle;
}

void *DynamicLibrary::getProcAddress(const std::string &name) {
	if(!isValid())
		return nullptr;

	void *func;
#ifdef EXTENSION_SYSTEM_OS_WINDOWS
	static_assert(sizeof(void *) == sizeof(void (*)(void)), "object pointer and function pointer sizes must be equal");
	auto tmp = GetProcAddress(_handle, name.c_str());
	func = *(void**)&tmp;
#else
	func = dlsym(_handle, name.c_str());
#endif
	if(func == nullptr)
		setLastError();
	return func;
}

std::string DynamicLibrary::fileExtension()
{
#ifdef EXTENSION_SYSTEM_OS_WINDOWS
	return ".dll";
#else
	return ".so";
#endif
}

bool DynamicLibrary::isValid() const {
	return _handle != nullptr;
}

void DynamicLibrary::setLastError() {
#ifdef EXTENSION_SYSTEM_OS_WINDOWS
	_last_error = GetLastError();
#else
	_last_error = dlerror();
#endif
}

std::string DynamicLibrary::getLastError() const {
	return _last_error;
}
