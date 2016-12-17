/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014-2016
		Distributed under the Boost Software License, Version 1.0.
		(See accompanying file LICENSE_1_0.txt or copy at
		http://www.boost.org/LICENSE_1_0.txt)
*/
#pragma once

// some general helper macros
#define EXTENSION_SYSTEM_UNUSED(arg) (void)arg;

#define EXTENSION_SYSTEM_STR_HELPER(x) #x
#define EXTENSION_SYSTEM_MACRO_EXPANSION_HELPER(x) x

#define EXTENSION_SYSTEM_STR(x) EXTENSION_SYSTEM_STR_HELPER(x)

#define EXTENSION_SYSTEM_CONCAT_HELPER2(a,b) a##b
#define EXTENSION_SYSTEM_CONCAT_HELPER1(a,b) EXTENSION_SYSTEM_CONCAT_HELPER2(a,b)
#define EXTENSION_SYSTEM_CONCAT(a,b) EXTENSION_SYSTEM_CONCAT_HELPER1(EXTENSION_SYSTEM_MACRO_EXPANSION_HELPER(a),EXTENSION_SYSTEM_MACRO_EXPANSION_HELPER(b))

// OS detection
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
	#define EXTENSION_SYSTEM_OS_WIN64
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#define EXTENSION_SYSTEM_OS_WIN32
#endif

#if defined(EXTENSION_SYSTEM_OS_WIN32) || defined(EXTENSION_SYSTEM_OS_WIN64)
	#define EXTENSION_SYSTEM_OS_WINDOWS
#elif defined(__linux__) || defined(__linux)
	#define EXTENSION_SYSTEM_OS_LINUX
#else
	#define Extend extensions_system/macros.hpp to support your OS
#endif

#define EXTENSION_SYSTEM_COMPILER_STR_MSVC "msvc"
#define EXTENSION_SYSTEM_COMPILER_STR_CLANG "clang"
#define EXTENSION_SYSTEM_COMPILER_STR_GPLUSPLUS "g++"

// compiler detection
#if defined(_MSC_VER)
	#define EXTENSION_SYSTEM_COMPILER_MSVC
	#define EXTENSION_SYSTEM_FUNCTION_NAME __FUNCSIG__

	#if defined(__INTEL_COMPILER)
		#define EXTENSION_SYSTEM_COMPILER_INTEL
	#else
		#define EXTENSION_SYSTEM_COMPILER EXTENSION_SYSTEM_COMPILER_STR_MSVC
		#define EXTENSION_SYSTEM_COMPILER_VERSION _MSC_VER
		#define EXTENSION_SYSTEM_COMPILER_VERSION_STR EXTENSION_SYSTEM_STR(_MSC_FULL_VER)
	#endif
#elif defined(__GNUC__)
	#define EXTENSION_SYSTEM_COMPILER_GNU
	#define EXTENSION_SYSTEM_FUNCTION_NAME __PRETTY_FUNCTION__

	#if defined(__MINGW32__)
		#define EXTENSION_SYSTEM_COMPILER_MINGW
	#endif

	#if defined(__INTEL_COMPILER)
		#define EXTENSION_SYSTEM_COMPILER_INTEL
	#elif defined(__clang__)
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

#if defined(EXTENSION_SYSTEM_COMPILER_INTEL)
	#define EXTENSION_SYSTEM_COMPILER_VERSION __INTEL_COMPILER
	#define EXTENSION_SYSTEM_COMPILER_VERSION_STR EXTENSION_SYSTEM_STR(__INTEL_COMPILER)
	#define EXTENSION_SYSTEM_COMPILER "intel"
#elif defined(EXTENSION_SYSTEM_COMPILER_GNU) && !defined(EXTENSION_SYSTEM_COMPILER_CLANG)
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

#ifdef EXTENSION_SYSTEM_OS_WINDOWS
	#define EXTENSION_SYSTEM_EXPORT __declspec(dllexport)
	#define EXTENSION_SYSTEM_IMPORT __declspec(dllimport)
	#define EXTENSION_SYSTEM_CDECL  __cdecl
#else
	#define EXTENSION_SYSTEM_EXPORT
	#define EXTENSION_SYSTEM_IMPORT
	#define EXTENSION_SYSTEM_CDECL
#endif

#if defined(EXTENSION_SYSTEM_COMPILER_INTEL)
	#error Please update the intel compiler path in extension_system/macros.hpp
	//#if EXTENSION_SYSTEM_COMPILER_VERSION < 1210
	//	#error Unsupported compiler, please use a newer intel compiler
	//#endif

	// doesn't work with icc (ICC) 13.0.1 20121010
	//#define EXTENSION_SYSTEM_COMPILER_SUPPORTS_EXPLICIT_OVERRIDE

	//#if EXTENSION_SYSTEM_COMPILER_VERSION >= 1300
	//	#define EXTENSION_SYSTEM_COMPILER_SUPPORTS_NOEXCEPT
	//	#define EXTENSION_SYSTEM_COMPILER_SUPPORTS_CONSTEXPR
	//#endif
#elif defined(EXTENSION_SYSTEM_COMPILER_CLANG)
	#if EXTENSION_SYSTEM_COMPILER_VERSION < 304
		#error Unsupported compiler, please use clang >= 3.4
	#endif

	#define EXTENSION_SYSTEM_COMPILER_SUPPORTS_NOEXCEPT
	#define EXTENSION_SYSTEM_COMPILER_SUPPORTS_CONSTEXPR
	#define EXTENSION_SYSTEM_COMPILER_SUPPORTS_NORETURN

#elif defined(EXTENSION_SYSTEM_COMPILER_GNU)
	#if EXTENSION_SYSTEM_COMPILER_VERSION < 408
		#error Unsupported compiler, please use a g++ >= 4.8
	#endif

	#define EXTENSION_SYSTEM_COMPILER_SUPPORTS_NOEXCEPT
	#define EXTENSION_SYSTEM_COMPILER_SUPPORTS_CONSTEXPR
	#define EXTENSION_SYSTEM_COMPILER_SUPPORTS_NORETURN

#elif defined(EXTENSION_SYSTEM_COMPILER_MSVC)
	#if EXTENSION_SYSTEM_COMPILER_VERSION < 1700
		#error Unsupported compiler, please use >=VC12
	#endif

	#define EXTENSION_SYSTEM_USE_STD_FILESYSTEM
#endif

// define the keywords


#ifdef EXTENSION_SYSTEM_COMPILER_SUPPORTS_NOEXCEPT
	#define EXTENSION_SYSTEM_NOEXCEPT noexcept
#else
	#define EXTENSION_SYSTEM_NOEXCEPT
#endif

#ifdef EXTENSION_SYSTEM_COMPILER_SUPPORTS_CONSTEXPR
	#define EXTENSION_SYSTEM_CONSTEXPR constexpr
#else
	#define EXTENSION_SYSTEM_CONSTEXPR
#endif

#ifdef EXTENSION_SYSTEM_COMPILER_SUPPORTS_NORETURN
	#define EXTENSION_SYSTEM_NORETURN [[noreturn]]
#else
	#define EXTENSION_SYSTEM_NORETURN
#endif
