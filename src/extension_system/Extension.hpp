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

#define  EXTENSION_SYSTEM_EXTENSION_API_VERSION 1

#define EXTENSION_SYSTEM_EXTENSION_DESCRIPTION_ENTRY(key,value) key "=" value "\0"

/**
 * You have to pass a fully qualified interface name (_interface).
 * your class should have a virtual destructor
 */
#define EXTENSION_SYSTEM_EXTENSION_EXT(_interface, _classname, _name, _version, _description, _user_defined, _function_name) \
	extern "C" EXTENSION_SYSTEM_EXPORT _interface* EXTENSION_SYSTEM_CDECL _function_name(_interface *, const char **); \
	extern "C" EXTENSION_SYSTEM_EXPORT _interface* EXTENSION_SYSTEM_CDECL _function_name(_interface *freeExtension, const char **data) { \
		const char extension_system_export[] = \
			EXTENSION_SYSTEM_EXTENSION_DESCRIPTION_ENTRY("EXTENSION_SYSTEM_METADATA_DESCRIPTION_START", EXTENSION_SYSTEM_STR(EXTENSION_SYSTEM_EXTENSION_API_VERSION)) \
			EXTENSION_SYSTEM_EXTENSION_DESCRIPTION_ENTRY("compiler", EXTENSION_SYSTEM_COMPILER) \
			EXTENSION_SYSTEM_EXTENSION_DESCRIPTION_ENTRY("compiler_version", EXTENSION_SYSTEM_COMPILER_VERSION_STR) \
			EXTENSION_SYSTEM_EXTENSION_DESCRIPTION_ENTRY("build_type", EXTENSION_SYSTEM_BUILD_TYPE) \
			EXTENSION_SYSTEM_EXTENSION_DESCRIPTION_ENTRY("interface_name", EXTENSION_SYSTEM_STR(_interface)) \
			EXTENSION_SYSTEM_EXTENSION_DESCRIPTION_ENTRY("name", _name) \
			EXTENSION_SYSTEM_EXTENSION_DESCRIPTION_ENTRY("version", EXTENSION_SYSTEM_STR(_version)) \
			EXTENSION_SYSTEM_EXTENSION_DESCRIPTION_ENTRY("description", _description) \
			EXTENSION_SYSTEM_EXTENSION_DESCRIPTION_ENTRY("entry_point", EXTENSION_SYSTEM_STR(_function_name)) \
			_user_defined \
			"EXTENSION_SYSTEM_METADATA_DESCRIPTION_END"; \
			if( freeExtension != nullptr ) {\
				delete freeExtension;\
				return nullptr;\
			} \
		if(data) \
			*data = extension_system_export; \
		return new _classname;\
	}

#define EXTENSION_SYSTEM_EXTENSION(_interface, _classname, _name, _version, _description, _user_defined) \
	EXTENSION_SYSTEM_EXTENSION_EXT(_interface, _classname, _name, _version, _description, _user_defined, \
		EXTENSION_SYSTEM_CONCAT(EXTENSION_SYSTEM_CONCAT(EXTENSION_SYSTEM_CONCAT(extension_system_entry_point_, __LINE__), _), __COUNTER__))

