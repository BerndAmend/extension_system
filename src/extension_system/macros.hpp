/**
    @file
    @copyright
        Copyright Bernd Amend and Michael Adam 2014-2017
        Distributed under the Boost Software License, Version 1.0.
        (See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt)
*/
#pragma once

// some general helper macros
// clang-format off
#define EXTENSION_SYSTEM_UNUSED(arg) (void)arg;

#define EXTENSION_SYSTEM_STR_HELPER(x) #x
#define EXTENSION_SYSTEM_MACRO_EXPANSION_HELPER(x) x

#define EXTENSION_SYSTEM_STR(x) EXTENSION_SYSTEM_STR_HELPER(x)

#define EXTENSION_SYSTEM_CONCAT_HELPER2(a,b) a##b
#define EXTENSION_SYSTEM_CONCAT_HELPER1(a,b) EXTENSION_SYSTEM_CONCAT_HELPER2(a,b)
#define EXTENSION_SYSTEM_CONCAT(a,b) EXTENSION_SYSTEM_CONCAT_HELPER1(EXTENSION_SYSTEM_MACRO_EXPANSION_HELPER(a),EXTENSION_SYSTEM_MACRO_EXPANSION_HELPER(b))

#define EXTENSION_SYSTEM_COMPILER_STR_MSVC "msvc"
#define EXTENSION_SYSTEM_COMPILER_STR_CLANG "clang"
#define EXTENSION_SYSTEM_COMPILER_STR_GPLUSPLUS "g++"

// compiler detection
#if defined(_MSC_VER)
    #define EXTENSION_SYSTEM_COMPILER_MSVC
    #define EXTENSION_SYSTEM_FUNCTION_NAME __FUNCSIG__

    #define EXTENSION_SYSTEM_COMPILER EXTENSION_SYSTEM_COMPILER_STR_MSVC
    #define EXTENSION_SYSTEM_COMPILER_VERSION _MSC_VER
    #define EXTENSION_SYSTEM_COMPILER_VERSION_STR EXTENSION_SYSTEM_STR(_MSC_FULL_VER)
#elif defined(__GNUC__)
    #define EXTENSION_SYSTEM_COMPILER_GNU
    #define EXTENSION_SYSTEM_FUNCTION_NAME __PRETTY_FUNCTION__

    #if defined(__MINGW32__)
        #define EXTENSION_SYSTEM_COMPILER_MINGW
    #endif

    #if defined(__clang__)
        #define EXTENSION_SYSTEM_COMPILER_CLANG
        #define EXTENSION_SYSTEM_COMPILER EXTENSION_SYSTEM_COMPILER_STR_CLANG
        #define EXTENSION_SYSTEM_COMPILER_VERSION ((__clang_major__ * 100) + __clang_minor__)
        #define EXTENSION_SYSTEM_COMPILER_VERSION_STR __VERSION__
    #else
        #define EXTENSION_SYSTEM_COMPILER_GPLUSPLUS
        #define EXTENSION_SYSTEM_COMPILER EXTENSION_SYSTEM_COMPILER_STR_GPLUSPLUS
    #endif

#else
    #error Unsupported compiler, extend extension_system/macros.hpp and the rest of the extension_system to support your compiler
#endif

#if defined(EXTENSION_SYSTEM_COMPILER_GNU) && !defined(EXTENSION_SYSTEM_COMPILER_CLANG)
    #define EXTENSION_SYSTEM_COMPILER_VERSION ((__GNUC__ * 100) + __GNUC_MINOR__)
    #define EXTENSION_SYSTEM_COMPILER_VERSION_STR __VERSION__
#endif

#define EXTENSION_SYSTEM_ENUM_TO_S(name) case name: return #name ;

#ifdef NDEBUG
    #ifdef _DEBUG
        #error _DEBUG and NDEBUG are defined, only one of them should be defined
    #endif
    #define EXTENSION_SYSTEM_BUILD_TYPE "release"
#else
    #define EXTENSION_SYSTEM_BUILD_TYPE "debug"
#endif

#ifdef _WIN32
    #define EXTENSION_SYSTEM_EXPORT __declspec(dllexport)
    #define EXTENSION_SYSTEM_IMPORT __declspec(dllimport)
    #define EXTENSION_SYSTEM_CDECL  __cdecl
#else
    #define EXTENSION_SYSTEM_EXPORT
    #define EXTENSION_SYSTEM_IMPORT
    #define EXTENSION_SYSTEM_CDECL
#endif

#if defined(EXTENSION_SYSTEM_COMPILER_CLANG)
    #if EXTENSION_SYSTEM_COMPILER_VERSION < 304
        #error Unsupported compiler, please use clang >= 3.4
    #endif

#elif defined(EXTENSION_SYSTEM_COMPILER_GNU)
    #if EXTENSION_SYSTEM_COMPILER_VERSION < 408
        #error Unsupported compiler, please use a g++ >= 4.8
    #endif

    // g++ >=7.1 has experimental filesystem support
    // sadly the implementation is by far slower than our simple posix wrapper
    // I verified that the function forEachFileInDirectory behaves identical
    // for both implementations (at least on linux)
    // in order to work the linker option -lstdc++fs is required
    // TODO: investigate why std::experimental is so slow
    //#if EXTENSION_SYSTEM_COMPILER_VERSION >= 701
    //    #define EXTENSION_SYSTEM_USE_STD_FILESYSTEM
    //#endif

#elif defined(EXTENSION_SYSTEM_COMPILER_MSVC)
    #if EXTENSION_SYSTEM_COMPILER_VERSION < 1700
        #error Unsupported compiler, please use >=VC12
    #endif

    #define EXTENSION_SYSTEM_USE_STD_FILESYSTEM
#endif
// clang-format off
