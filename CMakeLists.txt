# Copyright Bernd Amend and Michael Adam 2014
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
cmake_minimum_required(VERSION 2.8)

project(ExtensionSystem)

if(CMAKE_PROJECT_NAME STREQUAL "ExtensionSystem")
	set(EXTENSION_SYSTEM_IS_STANDALONE ON)
endif()

if(EXTENSION_SYSTEM_IS_STANDALONE)
	# Set a default build type if and only if user did not define one as command
	# line options and he did not give custom CFLAGS or CXXFLAGS. Otherwise, flags
	# from default build type would overwrite user-defined ones.
	if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_C_FLAGS AND NOT CMAKE_CXX_FLAGS)
		set(CMAKE_BUILD_TYPE Debug)
	endif ()

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
	elseif(MSVC11 OR MSVC12)
		set(EXTENSION_SYSTEM_COMPILER_MSVC ON)
	else()
		message(FATAL_ERROR "C++11 features are required. Please verify if extension_system is working with your compiler and send a bug report to the extension_system developers")
	endif()

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
		option(EXTENSION_SYSTEM_INTEL_STATIC_ANALYSIS "sets all required flags for the intel compiler to generate static analysis information" OFF)
		if(EXTENSION_SYSTEM_INTEL_STATIC_ANALYSIS)
			add_definitions(-diag-enable sc3)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -diag-enable sc3")
			set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -diag-enable sc3")
			set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -diag-enable sc3")
		endif()
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
endif()

include_directories(src)

# library
add_library(ExtensionSystem STATIC
						src/extension_system/DynamicLibrary.cpp
						src/extension_system/Extension.hpp			#PUBLIC_HEADER
						src/extension_system/ExtensionSystem.hpp	#PUBLIC_HEADER
						src/extension_system/filesystem.cpp
						src/extension_system/macros.hpp				#PUBLIC_HEADER
						src/extension_system/DynamicLibrary.hpp		#PUBLIC_HEADER
						src/extension_system/ExtensionSystem.cpp
						src/extension_system/filesystem.hpp
						src/extension_system/string.hpp)
target_link_libraries(ExtensionSystem ${CMAKE_DL_LIBS})

if(EXTENSION_SYSTEM_IS_STANDALONE)
	# Test library
	add_library(ExtensionSystemTestLib SHARED test/extension.cpp test/Interfaces.hpp)

	# Test program
	add_executable(ExtensionSystemTest test/main.cpp test/Interfaces.hpp)
	target_link_libraries(ExtensionSystemTest ExtensionSystem)

	if(EXTENSION_SYSTEM_COMPILER_MSVC)
		set_target_properties(ExtensionSystem PROPERTIES DEBUG_POSTFIX d)
		set_target_properties(ExtensionSystemTestLib PROPERTIES DEBUG_POSTFIX d)
		set_target_properties(ExtensionSystemTest PROPERTIES DEBUG_POSTFIX d)
	endif()

	install(TARGETS ExtensionSystem
		ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib
		RUNTIME DESTINATION bin)


	find_package(Doxygen)
	if (DOXYGEN_FOUND)
		add_custom_target (doc
							COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/Doxyfile
							SOURCES ${PROJECT_SOURCE_DIR}/Doxyfile)
	else()
		message("Doxygen is needed to build the documentation. Please install it to build the documentation.")
	endif()

	# CPACK
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Extension System")
	set(CPACK_PACKAGE_VENDOR "Bernd Amend and Michael Adam")
	#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ReadMe.txt")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE_1_0.txt")
	set(CPACK_PACKAGE_VERSION_MAJOR "1")
	# TODO
	include(CPack)
endif()