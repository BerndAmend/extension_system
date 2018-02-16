# Copyright Bernd Amend and Michael Adam 2014-2018
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
cmake_minimum_required(VERSION 3.1)

project(extension_system)

if(CMAKE_PROJECT_NAME STREQUAL "extension_system")
    set(EXTENSION_SYSTEM_IS_STANDALONE ON)
    set(CMAKE_CXX_STANDARD 11)

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wnon-virtual-dtor
                            -Woverloaded-virtual
                            $<$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,5.0>:-pedantic>
                            $<$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,4.9>:-fstack-protector-strong>
                            $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,5.0>:-Wno-missing-field-initializers>
                        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^.*Clang$")
        add_compile_options(-Wno-c++98-compat -Wno-c++98-compat-pedantic
                        -Wno-global-constructors -Wno-exit-time-destructors
                        -Wno-documentation
                        -Wno-padded
                        -Wno-weak-vtables
                        -Wno-reserved-id-macro
                        -Wno-attributes -Weverything)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)

        # disable some annoying compiler warnings
        add_compile_options(/wd4275) # disable warning C4275: non dll-interface x used as base for dll-interface y
        add_compile_options(/wd4251) # disable warning C4251: x needs to have dll-interface to be used by clients of y
        add_compile_options(/wd4068) # disable warning C4068: unknown pragma

        # enable additional compiler warnings
        add_compile_options(/w14062 /w14263 /w14264 /w14289 /w14706)
    else()
        message(FATAL_ERROR "Unsupported or unknown compiler ${CMAKE_CXX_COMPILER_ID}")
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "^.*Clang$")
        add_compile_options(-foptimize-sibling-calls)
        add_compile_options(-Wall -Wextra -Wno-unknown-pragmas -Wwrite-strings -Wenum-compare
            -Wno-conversion-null -Werror=return-type)
        add_compile_options($<$<CONFIG:Release>:-D_FORTIFY_SOURCE=2>)

        if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -z now -pie")

            set(EXTENSION_SYSTEM_SANITIZE "" CACHE STRING "Sanitizer not all options are available in all compiler versions")
            set_property(CACHE EXTENSION_SYSTEM_SANITIZE PROPERTY STRINGS "" none address memory thread undefined leak)
            if(NOT "${EXTENSION_SYSTEM_SANITIZE}" STREQUAL "")
                add_compile_options(-fsanitize=${EXTENSION_SYSTEM_SANITIZE} -fno-omit-frame-pointer)
            endif()
        endif()
    endif()

    # detect android
    include(CheckCXXSourceCompiles)
    CHECK_CXX_SOURCE_COMPILES("
        #ifndef __ANDROID__
            #error Android
        #endif
    " EXTENSION_SYSTEM_ANDROID)

    option(EXTENSION_SYSTEM_ENABLE_CLANG_TIDY "" OFF)
    if(EXTENSION_SYSTEM_ENABLE_CLANG_TIDY)
        find_program(
            CLANG_TIDY_EXE
            NAMES "clang-tidy"
            DOC "Path to clang-tidy executable"
            )
        if(CLANG_TIDY_EXE)
            message(STATUS "Enable clang-tidy: ${CLANG_TIDY_EXE}")
            set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}" "-fix" "-checks=*,-cppcoreguidelines-pro-bounds-*,-cppcoreguidelines-pro-type-vararg,-google-build-using-namespace")
        endif()
    endif()
endif()

# library
set(EXTENSION_SYSTEM_PUBLIC_HEADERS
                        src/extension_system/macros.hpp
                        src/extension_system/Extension.hpp
                        src/extension_system/DynamicLibrary.hpp
                        src/extension_system/ExtensionSystem.hpp
                        )
add_library(extension_system_headers INTERFACE)
target_include_directories(extension_system_headers INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
)

add_library(extension_system STATIC
                        ${EXTENSION_SYSTEM_PUBLIC_HEADERS}
                        src/extension_system/DynamicLibrary.cpp
                        src/extension_system/filesystem.cpp
                        src/extension_system/ExtensionSystem.cpp
                        src/extension_system/filesystem.hpp
                        src/extension_system/string.hpp)
target_link_libraries(extension_system PUBLIC extension_system_headers
                                       INTERFACE ${CMAKE_DL_LIBS})

option(EXTENSION_SYSTEM_DISABLE_BOOST "" OFF)
if(NOT EXTENSION_SYSTEM_DISABLE_BOOST)
    find_package(Boost QUIET)
    if(Boost_FOUND)
        message(STATUS "Use boost (${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}) to speed-up searching for extensions")
        target_compile_definitions(extension_system PRIVATE -DEXTENSION_SYSTEM_USE_BOOST)
        target_link_libraries(extension_system PRIVATE Boost::boost)
    endif()
endif()

set_target_properties(extension_system PROPERTIES PUBLIC_HEADER "${EXTENSION_SYSTEM_PUBLIC_HEADERS}")

if(EXTENSION_SYSTEM_IS_STANDALONE)
    enable_testing()

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dummy_test_extension
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    # Test library
    add_library(extension_system_test_lib SHARED test/extension.cpp test/Interfaces.hpp)
    target_link_libraries(extension_system_test_lib PRIVATE extension_system_headers)

    # Test program
    add_executable(extension_system_test test/main.cpp test/Interfaces.hpp test/catch.hpp)
    target_link_libraries(extension_system_test extension_system)
    add_test(extension_system_test extension_system_test -r junit -o juint.xml -s)

    # Examples
    ## Example 1
    add_library(extension_system_example1_extension SHARED examples/example1/Extension.cpp examples/example1/Interface.hpp)
    target_link_libraries(extension_system_example1_extension PRIVATE extension_system_headers)

    add_executable(extension_system_example1 examples/example1/main.cpp examples/example1/Interface.hpp)
    target_link_libraries(extension_system_example1 extension_system)

    ## Example 2
    add_library(extension_system_example2_extension SHARED examples/example2/Extension.cpp examples/example2/Interface.hpp)
    target_link_libraries(extension_system_example2_extension PRIVATE extension_system_headers)

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

    find_package(Doxygen QUIET)
    if (DOXYGEN_FOUND)
        add_custom_target (doc
                            COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/Doxyfile
                            SOURCES ${PROJECT_SOURCE_DIR}/Doxyfile
                            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    endif()
endif()
