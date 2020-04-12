/// SPDX-FileCopyrightText: 2014-2020 Bernd Amend and Michael Adam
/// SPDX-License-Identifier: BSL-1.0
#include "DynamicLibrary.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NO_STRICT
#define NO_STRICT
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef NOMINMAX
#define NOMINMAX // force windows.h not to define min and max
#endif
#include <windows.h>
#else // posix e.g. linux
#include <dlfcn.h>
#endif

using extension_system::DynamicLibrary;

DynamicLibrary::DynamicLibrary(const std::string& filename)
    : m_filename(filename) {
#ifdef _WIN32
    m_handle = LoadLibraryA(filename.c_str());
#else
    m_handle = dlopen(filename.c_str(), RTLD_LAZY | RTLD_NODELETE);
#endif
    if (m_handle == nullptr)
        setLastError();
}

DynamicLibrary::~DynamicLibrary() noexcept {
    if (isValid()) {
#ifdef _WIN32
        FreeLibrary(m_handle);
#else
        dlclose(m_handle);
#endif
    }
}

std::string DynamicLibrary::getFilename() const {
    return m_filename;
}

const void* DynamicLibrary::getHandle() const {
    return m_handle;
}

void* DynamicLibrary::getProcAddress(const std::string& name) {
    if (!isValid())
        return nullptr;

    void* func;
#ifdef _WIN32
    static_assert(sizeof(void*) == sizeof(void (*)(void)), "object pointer and function pointer sizes must be equal");
    const auto tmp = GetProcAddress(m_handle, name.c_str());
    func           = *(void**)&tmp;
#else
    func = dlsym(m_handle, name.c_str());
#endif
    if (func == nullptr)
        setLastError();
    return func;
}

std::string DynamicLibrary::fileExtension() {
#if defined(_WIN32)
    return ".dll";
#elif defined(__APPLE__)
    return ".dylib";
#else
    return ".so";
#endif
}

bool DynamicLibrary::isValid() const {
    return m_handle != nullptr;
}

void DynamicLibrary::setLastError() {
#ifdef _WIN32
    m_last_error = std::to_string(GetLastError());
#else
    auto err     = dlerror();
    m_last_error = err != nullptr ? err : "";
#endif
}

std::string DynamicLibrary::getError() const {
    return m_last_error;
}
