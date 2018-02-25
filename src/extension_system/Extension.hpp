/**
    @file
    @copyright
        Copyright Bernd Amend and Michael Adam 2014-2018
        Distributed under the Boost Software License, Version 1.0.
        (See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt)
*/
#pragma once

#ifdef _WIN32
#define EXTENSION_SYSTEM_EXPORT __declspec(dllexport)
#define EXTENSION_SYSTEM_CDECL __cdecl
#else
#define EXTENSION_SYSTEM_EXPORT
#define EXTENSION_SYSTEM_CDECL
#endif

#ifdef NDEBUG
#ifdef _DEBUG
#error _DEBUG and NDEBUG are defined, only one of them should be defined
#endif
#define EXTENSION_SYSTEM_BUILD_TYPE "release"
#else
#define EXTENSION_SYSTEM_BUILD_TYPE "debug"
#endif

#define EXTENSION_SYSTEM_STR_HELPER(x) #x
#define EXTENSION_SYSTEM_MACRO_EXPANSION_HELPER(x) x

#define EXTENSION_SYSTEM_STR(x) EXTENSION_SYSTEM_STR_HELPER(x)

#define EXTENSION_SYSTEM_CONCAT_HELPER2(a, b) a##b
#define EXTENSION_SYSTEM_CONCAT_HELPER1(a, b) EXTENSION_SYSTEM_CONCAT_HELPER2(a, b)
#define EXTENSION_SYSTEM_CONCAT(a, b) \
    EXTENSION_SYSTEM_CONCAT_HELPER1(EXTENSION_SYSTEM_MACRO_EXPANSION_HELPER(a), EXTENSION_SYSTEM_MACRO_EXPANSION_HELPER(b))

#define EXTENSION_SYSTEM_COMPILER_STR_MSVC "msvc"
#define EXTENSION_SYSTEM_COMPILER_STR_CLANG "clang"
#define EXTENSION_SYSTEM_COMPILER_STR_GPLUSPLUS "g++"

// clang-format off
#if defined(_MSC_VER)
    #define EXTENSION_SYSTEM_COMPILER EXTENSION_SYSTEM_COMPILER_STR_MSVC
    #define EXTENSION_SYSTEM_COMPILER_VERSION_STR EXTENSION_SYSTEM_STR(_MSC_FULL_VER)
#elif defined(__GNUC__)
    #if defined(__clang__)
        #define EXTENSION_SYSTEM_COMPILER EXTENSION_SYSTEM_COMPILER_STR_CLANG
        #define EXTENSION_SYSTEM_COMPILER_VERSION_STR __VERSION__
    #else
        #define EXTENSION_SYSTEM_COMPILER EXTENSION_SYSTEM_COMPILER_STR_GPLUSPLUS
        #define EXTENSION_SYSTEM_COMPILER_VERSION_STR __VERSION__
    #endif
#endif

#define EXTENSION_SYSTEM_EXTENSION_API_VERSION 1
#define EXTENSION_SYSTEM_EXTENSION_API_VERSION_STR EXTENSION_SYSTEM_STR(EXTENSION_SYSTEM_EXTENSION_API_VERSION)

#define EXTENSION_SYSTEM_DESCRIPTION_ENTRY(key, value) key "=" value "\0"

/**
 * You have to pass a fully qualified interface name (_interface).
 * your class should have a virtual destructor
 */
#define EXTENSION_SYSTEM_EXTENSION_EXT(_interface, _classname, _name, _version, _description, _user_defined, _function_name) \
    extern "C" EXTENSION_SYSTEM_EXPORT _interface* EXTENSION_SYSTEM_CDECL _function_name(_interface *, const char **); \
    extern "C" EXTENSION_SYSTEM_EXPORT _interface* EXTENSION_SYSTEM_CDECL _function_name(_interface *freeExtension, const char **data) { \
        const char *extension_system_export = \
            EXTENSION_SYSTEM_DESCRIPTION_ENTRY("EXTENSION_SYSTEM_METADATA_DESCRIPTION" "_START", EXTENSION_SYSTEM_EXTENSION_API_VERSION_STR) \
            EXTENSION_SYSTEM_DESCRIPTION_ENTRY("compiler", EXTENSION_SYSTEM_COMPILER) \
            EXTENSION_SYSTEM_DESCRIPTION_ENTRY("compiler_version", EXTENSION_SYSTEM_COMPILER_VERSION_STR) \
            EXTENSION_SYSTEM_DESCRIPTION_ENTRY("build_type", EXTENSION_SYSTEM_BUILD_TYPE) \
            EXTENSION_SYSTEM_DESCRIPTION_ENTRY("interface_name", EXTENSION_SYSTEM_STR(_interface)) \
            EXTENSION_SYSTEM_DESCRIPTION_ENTRY("name", _name) \
            EXTENSION_SYSTEM_DESCRIPTION_ENTRY("version", EXTENSION_SYSTEM_STR(_version)) \
            EXTENSION_SYSTEM_DESCRIPTION_ENTRY("description", _description) \
            EXTENSION_SYSTEM_DESCRIPTION_ENTRY("entry_point", EXTENSION_SYSTEM_STR(_function_name)) \
            _user_defined \
            "EXTENSION_SYSTEM_METADATA_DESCRIPTION" "_END"; \
            if( freeExtension != nullptr ) {\
                delete freeExtension;\
                return nullptr;\
            } \
        if(data) \
            *data = extension_system_export; \
        return new _classname;\
    }

/// Helper macro to make extension export more readable
#define EXTENSION_SYSTEM_NO_USER_DATA ""

/**
 * Export an extension.
 * \param _inteface fully qualified name of interface class (name with all namespaces)
 * \param _classname name of class to be exported
 * \param _name name of extension, used to instantiate an extension
 * \param _version version number of extension (by default extension system instantiates the highest version of an extension)
 * \param _description description of extension
 * \param _user_defined user defined metadata. Metadata can be defined by not comma separated calls of EXTENSION_SYSTEM_DESCRIPTION_ENTRY.
 *        If no user defined metadata is necessary, use EXTENSION_SYSTEM_NO_USER_DATA.
 */
#define EXTENSION_SYSTEM_EXTENSION(_interface, _classname, _name, _version, _description, _user_defined) \
    EXTENSION_SYSTEM_EXTENSION_EXT(_interface, _classname, _name, _version, _description, _user_defined, \
        EXTENSION_SYSTEM_CONCAT(EXTENSION_SYSTEM_CONCAT(EXTENSION_SYSTEM_CONCAT(extension_system_entry_point_, __LINE__), _), __COUNTER__))

// clang-format on

// Interface helper
namespace extension_system {
template <typename T>
struct InterfaceName {
    // You have to call EXTENSION_SYSTEM_INTERFACE(T) for your interface
};
}

/**
 * Make an interface known to extension_system.
 * In order to create extensions implementing a certain interface, this interface has to be exportet using EXTENSION_SYSTEM_INTERFACE macro
 * You have to call this macro with the fully qualified typename in the root namespace!
 */
#define EXTENSION_SYSTEM_INTERFACE(T)                     \
    namespace extension_system {                          \
    template <>                                           \
    struct InterfaceName<T> {                             \
        inline constexpr static const char* getString() { \
            return #T;                                    \
        }                                                 \
    };                                                    \
    }
