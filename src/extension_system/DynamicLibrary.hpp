/// SPDX-FileCopyrightText: 2014-2020 Bernd Amend and Michael Adam
/// SPDX-License-Identifier: BSL-1.0
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
    std::string m_filename;
    void*       m_handle{};
    std::string m_last_error;

    void setLastError();
};
}
