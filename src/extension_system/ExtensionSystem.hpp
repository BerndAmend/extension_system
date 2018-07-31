/**
   @file
   @copyright
       Copyright Bernd Amend and Michael Adam 2014-2018
       Distributed under the Boost Software License, Version 1.0.
       (See accompanying file LICENSE_1_0.txt or copy at
       http://www.boost.org/LICENSE_1_0.txt)
*/
#pragma once

#include <sstream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>

#include "Extension.hpp"
#include "DynamicLibrary.hpp"

namespace extension_system {

using ExtensionVersion = uint32_t;

/**
 * Structure that describes an extenstion.
 * Basically an extension is described by
 * \li the interface it implements,
 * \li its name and
 * \li its version.
 * Additionally a extension creator can add metadata to further describe the extension.
 * The handling of extensions with same name and version number in different libraries is currently broken
 */
class ExtensionDescription final {
public:
    ExtensionDescription()                            = default;
    ExtensionDescription(ExtensionDescription&&)      = default;
    ExtensionDescription(const ExtensionDescription&) = default;
    ExtensionDescription& operator=(ExtensionDescription&&) = default;
    ExtensionDescription& operator=(const ExtensionDescription&) = default;
    ~ExtensionDescription() noexcept                             = default;

    ExtensionDescription(std::unordered_map<std::string, std::string>&& data, ExtensionVersion version)
        : _data{data}
        , _version{version} {}

    /**
     * Returns if the extension is valid. An extension is invalid if the describing data structure was not found within the shared module
     * (.so/.dll ...)
     */
    bool isValid() const {
        return !_data.empty();
    }

    /**
     * Gets the extensions name. The name is given by extension's author.
     */
    std::string name() const {
        return get("name");
    }

    /**
     * Gets the version of the extension.
     * @return the version or 0 if the value couldn't be parsed or didn't exist
     */
    ExtensionVersion version() const {
        return _version;
    }

    /**
     * Returns extensions description.
     */
    std::string description() const {
        return get("description");
    }

    /**
     * Returns the fully qualified interface name the extension implements.
     */
    std::string interface_name() const {
        return get("interface_name");
    }

    /**
     * Returns file name of the library containing the extension.
     */
    std::string library_filename() const {
        return get("library_filename");
    }

    /**
     * Returns a map of additional metadata, defined by extension's author
     */
    const std::unordered_map<std::string, std::string>& data() const {
        return _data;
    }

    /**
     * Returns meta data identified by key
     * @param key Key to retrieve metadata for
     * @return Metadata associated with key or an empty string if key was not found in meta data
     */
    std::string get(const std::string& key) const {
        auto iter = _data.find(key);
        if (iter == _data.end())
            return {};
        return iter->second;
    }

    /**
     * Equivalent to get()
     */
    std::string operator[](const std::string& key) const {
        return get(key);
    }

    bool operator==(const ExtensionDescription& desc) const {
        return _data == desc._data && _version == desc._version;
    }

private:
    std::unordered_map<std::string, std::string> _data;
    ExtensionVersion                             _version{};
};

inline std::string to_string(const ExtensionDescription& e) {
    std::stringstream out;
    for (const auto& iter : e.data())
        out << "  " << iter.first << " = " << iter.second << "\n";
    return out.str();
}

/**
 * @brief The ExtensionSystem class
 * thread-safe
 */
class ExtensionSystem final {
public:
    ExtensionSystem();
    ExtensionSystem(ExtensionSystem&&)      = delete;
    ExtensionSystem(const ExtensionSystem&) = delete;
    ExtensionSystem& operator=(ExtensionSystem&&) = delete;
    ExtensionSystem& operator=(const ExtensionSystem&) = delete;

    ~ExtensionSystem() noexcept;

    /**
     * Scans a dynamic library file for extensions and adds these extensions to the list of known extensions.
     * @param filename File name of the library
     * @return number of extensions found in the file
     */
    std::size_t addDynamicLibrary(const std::string& filename);

    /**
     * Removes all extensions provided by the library from the list of known extensions
     * Currently instantiated extensions are not affected by this call
     * @param filename File name of library to remove
     */
    void removeDynamicLibrary(const std::string& filename);

    /**
     * Scans a directory for dynamic libraries and calls addDynamicLibrary for every found library
     * @param path Path to search in for extensions
     * @param recursive Search in subdirectories
     */
    void searchDirectory(const std::string& path, bool recursive = false);

    /**
     * Scans a directory for dynamic libraries and calls addDynamicLibrary for every found library whose file begins with required_prefix
     * @param path Path to search in for extensions
     * @param required_prefix Required prefix for libraries
     * @param recursive Search in subdirectories
     */
    void searchDirectory(const std::string& path, const std::string& required_prefix, bool recursive);

    /**
     * Returns a list of all known extensions
     */
    std::vector<ExtensionDescription> extensions() const;

    /**
     * Returns a list of extensions, filtered by metadata.
     * Same metadata keys are treated as or-linked, different keys are and-linked treated.
     * For example: {{"author", "Alice"}, {"author", "Bob"}, {"company", "MyCorp"}} will list all extensions whose authors are Alice or Bob
     * and the company is "MyCorp"
     * @param metaDataFilter Metadata to search extensions for. Use c++11 initializer lists for simple usage: {{"author", "Alice"},
     * {"company", "MyCorp"}}
     * @return List of extensions
     */
    std::vector<ExtensionDescription> extensions(const std::vector<std::pair<std::string, std::string>>& metaDataFilter) const;

    /**
     * Returns a list of all known extensions of a specified interface type
     */
    template <class T>
    std::vector<ExtensionDescription> extensions(std::vector<std::pair<std::string, std::string>> metaDataFilter = {}) const {
        metaDataFilter.emplace_back("interface_name", extension_system::InterfaceName<T>::getString());
        return extensions(metaDataFilter);
    }

    /**
     * Creates an instance of an extension with a specified version
     * Instantiated extension can outlive the ExtensionSystem (instance)
     * @param name Name of extension to create
     * @param version Version of extension to create
     * @return An instance of an extension class or nullptr, if extension could not be instantiated
     */
    template <class T>
    std::shared_ptr<T> createExtension(const std::string& name, ExtensionVersion version) {
        std::unique_lock<std::mutex> lock{_mutex};
        auto                         desc = findDescriptionUnsafe(extension_system::InterfaceName<T>::getString(), name, version);
        if (!desc.isValid())
            return {};
        return createExtensionUnsafe<T>(desc);
    }

    /**
     * Creates an instance of an extension. If the extension is available in multiple versions, the highest version will be instantiated
     * Instantiated extension can outlive the ExtensionSystem (instance)
     * @param name Name of extension to create
     * @return An instance of an extension class or nullptr, if extension could not be instantiated
     */
    template <class T>
    std::shared_ptr<T> createExtension(const std::string& name) {
        std::unique_lock<std::mutex> lock{_mutex};
        const auto                   desc = findDescriptionUnsafe(extension_system::InterfaceName<T>::getString(), name);
        if (!desc.isValid())
            return {};
        return createExtensionUnsafe<T>(desc);
    }

    /**
     * Creates an instance of an extension using an ExtensionDescription.
     * Instantiated extension can outlive the ExtensionSystem (instance)
     * @param desc desc as returned by extension(...)
     * @return an instance of an extension class or a nullptr, if extension could not be instantiated
     */
    template <class T>
    std::shared_ptr<T> createExtension(const ExtensionDescription& desc) {
        std::unique_lock<std::mutex> lock{_mutex};
        return createExtensionUnsafe<T>(desc);
    }

    /**
     * Sets a message handler.
     * A message handler is a function that should be called if the ExtensionSystem detects an non fatal error while adding a library.
     * The default message handler prints to std::cerr
     * @param func Message handler function or nullptr if messages should be disabled.
     */
    void setMessageHandler(const std::function<void(const std::string&)>& func) {
        if (func == nullptr)
            _message_handler = [](const std::string&) {};
        else
            _message_handler = func;
    }

    void setMessageHandler(std::nullptr_t) {
        _message_handler = [](const std::string&) {};
    }

    bool getVerifyCompiler() const {
        return _verify_compiler;
    }

    /**
     * enables or disables the compiler match verification
     * only effects libraries that are added afterwards
     */
    void setVerifyCompiler(bool enable);

    void setEnableDebugOutput(bool enable);

private:
    std::size_t addDynamicLibrary(const std::string& filename, std::vector<char>& buffer);
    std::size_t addExtensions(const std::string& filename, const char* file_content, std::size_t file_length);
    std::unordered_map<std::string, std::string> parseKeyValue(const std::string& filename, const char* start, const char* end);
    ExtensionDescription                         parse(const std::string& filename, std::unordered_map<std::string, std::string>&& desc);

    ExtensionDescription findDescriptionUnsafe(const std::string& interface_name, const std::string& name, ExtensionVersion version) const;
    ExtensionDescription findDescriptionUnsafe(const std::string& interface_name, const std::string& name) const;

    template <class T>
    std::shared_ptr<T> createExtensionUnsafe(const ExtensionDescription& desc) {
        if (!desc.isValid() || extension_system::InterfaceName<T>::getString() != desc.interface_name())
            return {};

        for (auto& i : _known_extensions) {
            for (auto& j : i.second.extensions) {
                if (j == desc) {
                    auto dynlib = i.second.dynamic_library.lock();
                    if (dynlib == nullptr) {
                        dynlib = std::make_shared<DynamicLibrary>(i.first);
                        if (!dynlib->isValid())
                            _message_handler("_createExtension: " + dynlib->getError());
                        i.second.dynamic_library = dynlib;
                    }

                    if (dynlib == nullptr)
                        continue;

                    const auto func = dynlib->getProcAddress<T*(T*, const char**)>(j.get("entry_point"));

                    if (func != nullptr) {
                        T* ex = func(nullptr, nullptr);
                        if (ex != nullptr) {
                            return std::shared_ptr<T>(ex, [dynlib, func](T* obj) { func(obj, nullptr); });
                        }
                    }
                }
            }
        }
        return {};
    }

    struct LibraryInfo final {
        LibraryInfo()                   = default;
        LibraryInfo(LibraryInfo&&)      = default;
        LibraryInfo(const LibraryInfo&) = delete;
        LibraryInfo& operator=(const LibraryInfo&) = delete;
        LibraryInfo& operator=(LibraryInfo&&) = default;
        ~LibraryInfo() noexcept               = default;
        explicit LibraryInfo(std::vector<ExtensionDescription> ex)
            : extensions{std::move(ex)} {}

        std::weak_ptr<DynamicLibrary>     dynamic_library;
        std::vector<ExtensionDescription> extensions;
    };

    void debugMessage(const std::string& msg);

    bool _verify_compiler = true;
    bool _debug_output    = false;

    std::function<void(const std::string&)>      _message_handler;
    mutable std::mutex                           _mutex;
    std::unordered_map<std::string, LibraryInfo> _known_extensions;

    // The following strings are used to find the exported classes in the dll/so files
    // The strings are concatenated at runtime to avoid that they are found in the ExtensionSystem binary.
    const std::string desc_base  = "EXTENSION_SYSTEM_METADATA_DESCRIPTION_";
    const std::string desc_start = ExtensionSystem::desc_base + "START";
    const std::string desc_end   = ExtensionSystem::desc_base + "END";
};
}
