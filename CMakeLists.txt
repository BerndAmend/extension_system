# Copyright Bernd Amend and Michael Adam 2014
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
cmake_minimum_required(VERSION 2.8)
if("${CMAKE_MAJOR_VERSION}" GREATER 2)
	cmake_policy(SET CMP0048 NEW)
endif()

project(extension_system)

if(CMAKE_PROJECT_NAME STREQUAL "extension_system")
	set(EXTENSION_SYSTEM_IS_STANDALONE ON)
endif()

if(EXTENSION_SYSTEM_IS_STANDALONE)
	set(EXTENSION_SYSTEM_CONFIGURE_COMPILER ON)
endif()

include("CompilerDetection.cmake")

include(CheckIncludeFileCXX)

CHECK_INCLUDE_FILE_CXX("boost/algorithm/searching/boyer_moore.hpp" EXTENSION_SYSTEM_USE_BOOST_BOYER_MOORE)

if(EXTENSION_SYSTEM_USE_BOOST_BOYER_MOORE)
	message(STATUS "Use boost/algorithm/searching/boyer_moore.hpp to speed-up searching for extensions")
	add_definitions(-DEXTENSION_SYSTEM_USE_BOOST_BOYER_MOORE)
endif()

include_directories(src)

# library
set(EXTENSION_SYSTEM_PUBLIC_HEADERS
						src/extension_system/macros.hpp
						src/extension_system/Extension.hpp
						src/extension_system/DynamicLibrary.hpp
						src/extension_system/ExtensionSystem.hpp
						)

add_library(extension_system STATIC
						${EXTENSION_SYSTEM_PUBLIC_HEADERS}
						src/extension_system/DynamicLibrary.cpp
						src/extension_system/filesystem.cpp
						src/extension_system/ExtensionSystem.cpp
						src/extension_system/filesystem.hpp
						src/extension_system/string.hpp)
target_link_libraries(extension_system ${CMAKE_DL_LIBS})

set_target_properties(extension_system PROPERTIES PUBLIC_HEADER "${EXTENSION_SYSTEM_PUBLIC_HEADERS}")

if(EXTENSION_SYSTEM_IS_STANDALONE)
	# Test library
	add_library(extension_system_test_lib SHARED test/extension.cpp test/Interfaces.hpp)

	# Test program
	add_executable(extension_system_test test/main.cpp test/Interfaces.hpp)
	target_link_libraries(extension_system_test extension_system)

	# Examples
	## Example 1
	add_library(extension_system_example1_extension SHARED examples/example1/Extension.cpp examples/example1/Interface.hpp)
	add_executable(extension_system_example1 examples/example1/main.cpp examples/example1/Interface.hpp)
	target_link_libraries(extension_system_example1 extension_system)

	## Example 2
	add_library(extension_system_example2_extension SHARED examples/example2/Extension.cpp examples/example2/Interface.hpp)
	add_executable(extension_system_example2 examples/example2/main.cpp examples/example2/Interface.hpp)
	target_link_libraries(extension_system_example2 extension_system)

	if(EXTENSION_SYSTEM_COMPILER_MSVC)
		set_target_properties(extension_system PROPERTIES DEBUG_POSTFIX d)
		set_target_properties(extension_system_test_lib PROPERTIES DEBUG_POSTFIX d)
		set_target_properties(extension_system_test PROPERTIES DEBUG_POSTFIX d)
		set_target_properties(extension_system_example1_extension PROPERTIES DEBUG_POSTFIX d)
		set_target_properties(extension_system_example1 PROPERTIES DEBUG_POSTFIX d)
		set_target_properties(extension_system_example2_extension PROPERTIES DEBUG_POSTFIX d)
		set_target_properties(extension_system_example2 PROPERTIES DEBUG_POSTFIX d)
	endif()

	if(EXTENSION_SYSTEM_ANDROID)
		target_link_libraries(extension_system_test_lib gnustl_shared)
		target_link_libraries(extension_system_test gnustl_shared)
		target_link_libraries(extension_system_example1_extension gnustl_shared)
		target_link_libraries(extension_system_example1 gnustl_shared)
		target_link_libraries(extension_system_example2_extension gnustl_shared)
		target_link_libraries(extension_system_example2 gnustl_shared)
	endif()

	install(TARGETS extension_system
		ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib
		RUNTIME DESTINATION bin
		PUBLIC_HEADER DESTINATION include/extension_system)


	find_package(Doxygen)
	if (DOXYGEN_FOUND)
		add_custom_target (doc
							COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/Doxyfile
							SOURCES ${PROJECT_SOURCE_DIR}/Doxyfile
							WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
	else()
		message("Doxygen is needed to build the documentation. Please install it to build the documentation.")
	endif()

	# CPACK
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "extension_system")
	set(CPACK_PACKAGE_VENDOR "Bernd Amend and Michael Adam")
	#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ReadMe.txt")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE_1_0.txt")
	set(CPACK_PACKAGE_VERSION_MAJOR "1")
	# TODO
	include(CPack)
endif()
