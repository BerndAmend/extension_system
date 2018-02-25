/**
    @file
    @copyright
        Copyright Bernd Amend and Michael Adam 2014-2018
        Distributed under the Boost Software License, Version 1.0.
        (See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt)
*/
#pragma once

#include <string>
#include <vector>
#include <functional>

#include "string.hpp"

#ifdef _MSC_VER
#define EXTENSION_SYSTEM_USE_STD_FILESYSTEM
#endif

// g++ >=7 has experimental filesystem support
// sadly the implementation is by far slower than our simple posix wrapper
// I verified that the function forEachFileInDirectory behaves identical
// for both implementations (at least on linux)
// in order to work the linker option -lstdc++fs is required
// TODO: investigate why std::experimental is so slow
//#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 7
//#define EXTENSION_SYSTEM_USE_STD_FILESYSTEM
//#endif

#ifdef EXTENSION_SYSTEM_USE_STD_FILESYSTEM
// clang-format off
#if defined(_MSC_VER) && _MSC_VER >= 1700
#include <filesystem>
#elif __has_include(<filesystem>)
#include <filesystem>
#else
#include <experimental/filesystem>
#endif

namespace extension_system {
namespace filesystem {
#if defined(_MSC_VER) && _MSC_VER >= 1700
#if _MSC_VER < 1912
using namespace std::tr2::sys;
#else
using namespace std::experimental::filesystem;
#endif
path canonical(const path& p);
#elif __has_include(<experimental/filesystem>)
using namespace std::experimental::filesystem;
#else
using namespace std::filesystem;
#endif
// clang-format on

void forEachFileInDirectory(const path& root, const std::function<void(const path& p)>& func, bool recursive);
}
}
#else
namespace extension_system {
namespace filesystem {
class path final {
public:
    path() = default;
    path(const path& p) {
        *this = p._pathname;
    }
    path(const char* p) {
        *this = std::string(p);
    }
    path(const std::string& p) {
        *this = p;
    }

    path& operator=(const path& p) {
        *this = p._pathname;
        return *this;
    }
    path& operator=(const std::string& p) {
        _pathname = p;
        return *this;
    }

    const std::string string() const {
        return _pathname;
    }
    // NOLINTNEXTLINE(readability-identifier-naming)
    const std::string generic_string() const {
        return _pathname;
    }

    path filename() const {
        path result;
        split(_pathname, '/', [&](const std::string& str) {
            result = str;
            return true;
        });
        return result;
    }

    path extension() const {
        const auto name = filename().string();
        if (name == "." || name == "..")
            return {};
        const auto pos = name.find_last_of('.');
        if (pos == std::string::npos)
            return {};
        return name.substr(pos);
    }

    path operator/(const std::string& rhs) const {
        return path(this->string() + "/" + rhs);
    }

    path operator/(const path& rhs) const {
        return (*this) / rhs.string();
    }

private:
    std::string _pathname;
};

bool exists(const path& p);
bool is_directory(const path& p);
path canonical(const path& p);

void forEachFileInDirectory(const path& root, const std::function<void(const filesystem::path& p)>& func, bool recursive);
}
}
#endif
