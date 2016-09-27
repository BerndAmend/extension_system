# Copyright Bernd Amend and Michael Adam 2014
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
cmake_minimum_required(VERSION 2.8)

if(NOT EXTENSION_SYSTEM_COMPILER_DETECTED)
	set(EXTENSION_SYSTEM_COMPILER_DETECTED ON)

	if("${CMAKE_CXX_COMPILER}" MATCHES ".*clang${plus}${plus}")
		set(EXTENSION_SYSTEM_COMPILER_CLANG ON)
	elseif("${CMAKE_CXX_COMPILER}" MATCHES ".*icc" OR "${CMAKE_CXX_COMPILER}" MATCHES ".*icpc")
		set(EXTENSION_SYSTEM_COMPILER_INTEL ON)
	elseif(CMAKE_COMPILER_IS_GNUCXX)
		set(EXTENSION_SYSTEM_COMPILER_GCC ON)
		if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
			message(FATAL_ERROR "C++11 features are required. Therefore a gcc compiler >=4.8 is needed.")
		endif()
	elseif(MSVC10 OR MSVC90 OR MSVC80 OR MSVC71 OR MSVC70 OR MSVC60)
		message(FATAL_ERROR "C++11 features are required. <=VS2010 doesn't support them")
	elseif(MSVC11 OR MSVC12 OR MSVC13 OR MSVC14)
		set(EXTENSION_SYSTEM_COMPILER_MSVC ON)
	else()
		message(FATAL_ERROR "C++11 features are required. Please verify if extension_system is working with your compiler and send a bug report to the extension_system developers")
	endif()
endif()


if(EXTENSION_SYSTEM_CONFIGURE_COMPILER AND NOT EXTENSION_SYSTEM_COMPILER_CONFIGURED)
	set(EXTENSION_SYSTEM_COMPILER_CONFIGURED ON)
	# Set a default build type if and only if user did not define one as command
	# line options and he did not give custom CFLAGS or CXXFLAGS. Otherwise, flags
	# from default build type would overwrite user-defined ones.
	if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_C_FLAGS AND NOT CMAKE_CXX_FLAGS)
		set(CMAKE_BUILD_TYPE Debug)
	endif ()

	if(EXTENSION_SYSTEM_COMPILER_GCC)
		add_definitions(-std=c++11 -foptimize-sibling-calls)
		add_definitions(-Wall -Wextra -Wno-unknown-pragmas -Wwrite-strings -Wenum-compare
						-Wno-conversion-null -Werror=return-type -pedantic -Wnon-virtual-dtor
						-Woverloaded-virtual)
	elseif(EXTENSION_SYSTEM_COMPILER_CLANG)
		add_definitions(-std=c++11 -foptimize-sibling-calls)
		add_definitions(-Wall -Wextra -Wno-unknown-pragmas -Wwrite-strings -Wenum-compare
						-Wno-conversion-null -Werror=return-type
						-Wno-c++98-compat -Wno-c++98-compat-pedantic
						-Wno-global-constructors -Wno-exit-time-destructors
						-Wno-documentation
						-Wno-padded
						-Wno-weak-vtables
						-Wno-attributes)
	# -Weverything triggers to many warnings in qt
	elseif(EXTENSION_SYSTEM_COMPILER_INTEL)
		add_definitions(-std=c++11)
		add_definitions(-Wall -Wextra -Wno-unknown-pragmas -Wwrite-strings)
	elseif(EXTENSION_SYSTEM_COMPILER_MSVC)
		# enable parallel builds
		add_definitions(/MP)

		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
		add_definitions(-DNOMINMAX) # force windows.h not to define min and max

		# disable some annoying compiler warnings
		add_definitions(/wd4275) # disable warning C4275: non dll-interface x used as base for dll-interface y
		add_definitions(/wd4251) # disable warning C4251: x needs to have dll-interface to be used by clients of y
		add_definitions(/wd4068) # disable warning C4068: unknown pragma

		# enable additional compiler warnings
		add_definitions(/w14062 /w14263 /w14264 /w14289 /w14623 /w14706)
	endif()

	if(NOT EXTENSION_SYSTEM_COMPILER_MSVC)
		set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)
	endif()

	if(EXTENSION_SYSTEM_COMPILER_GCC OR EXTENSION_SYSTEM_COMPILER_CLANG)
		set(EXTENSION_SYSTEM_SANITIZE "" CACHE STRING "Sanitizer not all options are available in all compiler versions")
		set_property(CACHE EXTENSION_SYSTEM_SANITIZE PROPERTY STRINGS "" address memory thread undefined leak)
		if ("${EXTENSION_SYSTEM_SANITIZE}" STREQUAL "address")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
		elseif("${EXTENSION_SYSTEM_SANITIZE}" STREQUAL "memory")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory -fno-omit-frame-pointer")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=memory")
		elseif("${EXTENSION_SYSTEM_SANITIZE}" STREQUAL "leak")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak -fno-omit-frame-pointer")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=leak")
		elseif("${EXTENSION_SYSTEM_SANITIZE}" STREQUAL "undefined")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
		elseif("${EXTENSION_SYSTEM_SANITIZE}" STREQUAL "thread")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer -fPIE")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread -fPIE -pie")
		endif()
	endif()

	# detect android
	include(CheckCXXSourceCompiles)
	CHECK_CXX_SOURCE_COMPILES("
		#ifndef __ANDROID__
			#error Android
		#endif
	" EXTENSION_SYSTEM_ANDROID)
endif()
