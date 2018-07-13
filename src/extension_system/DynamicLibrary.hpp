/**
    @file
    @copyright
        Copyright Bernd Amend and Michael Adam 2014-2018
        Distributed under the Boost Software License, Version 1.0.
        (See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt)
*/
#pragma once

#include <functional>
#include <string>

namespace extension_system {

class DynamicLibrary final {
public:
    DynamicLibrary() = default;
    explicit DynamicLibrary(const std::string& filename);
    DynamicLibrary(DynamicLibrary&&)      = default;
    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(DynamicLibrary&&) = default;
    DynamicLibrary& operator=(const DynamicLibrary&) noexcept = delete;
    ~DynamicLibrary() noexcept;

    std::string getFilename() const;

    const void* getHandle() const;

    void* getProcAddress(const std::string& name);

    template <typename FunctionSignature>
    std::function<FunctionSignature> getProcAddress(const std::string& name) {
        return reinterpret_cast<FunctionSignature*>(getProcAddress(name));
    }

    static std::string fileExtension();

    bool isValid() const;

    std::string getError() const;

private:
    std::string _filename;
    void*       _handle = nullptr;
    std::string _last_error;

    void setLastError();
};
}
