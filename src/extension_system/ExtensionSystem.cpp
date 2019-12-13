/// SPDX-FileCopyrightText: 2014-2020 Bernd Amend and Michael Adam
/// SPDX-License-Identifier: BSL-1.0
#include "ExtensionSystem.hpp"

#include "filesystem.hpp"
#include "string.hpp"
#include <algorithm>
#include <iostream>
#include <unordered_set>

#ifdef EXTENSION_SYSTEM_USE_BOOST
#ifdef _WIN32
#define BOOST_DATE_TIME_NO_LIB
#endif
#include <boost/algorithm/searching/boyer_moore.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#else
#include <fstream>
#endif

using namespace extension_system;

namespace {
inline std::string getRealFilename(const std::string& filename) {
    filesystem::path filen{filename};

    if (!filesystem::exists(filen)) { // check if the file exists
        filen = filesystem::path{filename + DynamicLibrary::fileExtension()};
        if (!filesystem::exists(filen))
            return {};
    }

    return canonical(filen).generic_string();
}

#ifdef EXTENSION_SYSTEM_USE_BOOST
using StringSearch = boost::algorithm::boyer_moore<const char*>;
#else
class StringSearch final {
public:
    StringSearch(const char* pattern_first, const char* pattern_last)
        : _pattern_first(pattern_first)
        , _pattern_last(pattern_last) {}

    const char* operator()(const char* first, const char* last) const {
        // HINT: memmem is much slower than std::search
        return std::search(first, last, _pattern_first, _pattern_last);
    }

private:
    const char* _pattern_first;
    const char* _pattern_last;
};
#endif

} // namespace

// boost broke compatibility
// https://svn.boost.org/trac/boost/ticket/12552
template <typename corpusIter>
inline corpusIter getFirstFromPair(const std::pair<corpusIter, corpusIter>& p) {
    return p.first;
}
template <typename corpusIter>
inline corpusIter getFirstFromPair(corpusIter p) {
    return p;
}

ExtensionSystem::ExtensionSystem()
    : _message_handler([](const std::string& msg) { std::cerr << "ExtensionSystem::" << msg << std::endl; }) {}

ExtensionSystem::~ExtensionSystem() noexcept = default;

std::size_t ExtensionSystem::addDynamicLibrary(const std::string& filename) {
    std::vector<char> buffer;
    return addDynamicLibrary(filename, buffer);
}

std::size_t ExtensionSystem::addDynamicLibrary(const std::string& filename, std::vector<char>& buffer) {
    debugMessage("check file " + filename);
    const std::string file_path = getRealFilename(filename);

    if (file_path.empty()) {
        _message_handler("addDynamicLibrary: neither " + filename + " nor " + filename + DynamicLibrary::fileExtension() + " exist.");
        return 0;
    }

    if (filesystem::is_directory(file_path)) {
        _message_handler("addDynamicLibrary: doesn't support adding directories directory=" + filename);
        return 0;
    }

    auto already_loaded = _known_extensions.find(file_path);

    // don't reload library
    if (already_loaded != _known_extensions.end())
        return 0;

    std::size_t file_length = 0;
    const char* file_content{};

#ifdef EXTENSION_SYSTEM_USE_BOOST
    (void)buffer;
    const boost::interprocess::mode_t mode = boost::interprocess::read_only;
    boost::interprocess::file_mapping fm;

    boost::interprocess::mapped_region file;

    try {
        fm = boost::interprocess::file_mapping(file_path.c_str(), mode);
    } catch (const boost::interprocess::interprocess_exception& e) {
        _message_handler(std::string("file_mapping failed ") + e.what());
        return 0;
    }

    try {
        file = boost::interprocess::mapped_region(fm, mode, 0, 0);
    } catch (const boost::interprocess::interprocess_exception& e) {
        _message_handler(std::string("mapped_region failed ") + e.what());
        return 0;
    }

    file_length  = file.get_size();
    file_content = reinterpret_cast<const char*>(file.get_address()); // NOLINT
#else
    {
        std::ifstream file;
        file.open(file_path, std::ios::in | std::ios::binary | std::ios::ate);

        if (!file) {
            _message_handler("couldn't open file");
            return 0;
        }

        if (file.tellg() <= 0) {
            _message_handler("invalid or unknown file size");
            return 0;
        }

        file_length = static_cast<std::size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        if (buffer.size() < static_cast<std::size_t>(file_length))
            buffer.resize(static_cast<std::size_t>(file_length));

        file_content = buffer.data();

        file.read(buffer.data(), static_cast<std::streamsize>(file_length));
    }
#endif

    return addExtensions(filename, file_content, file_length);
}

std::size_t ExtensionSystem::addExtensions(const std::string& filename, const char* file_content, std::size_t file_length) {
    StringSearch search_start(desc_start.c_str(), desc_start.c_str() + desc_start.length());
    StringSearch search_end{desc_end.c_str(), desc_end.c_str() + desc_end.length()};

    auto        file_path = getRealFilename(filename);
    LibraryInfo info;

    const char* file_end = file_content + file_length;
    for (const char* current = getFirstFromPair(search_start(file_content, file_end)); current != file_end;
         current             = getFirstFromPair(search_start(current, file_end))) {

        const char* start = current;

        current         = getFirstFromPair(search_end(current + 1, file_end));
        const char* end = current;

        if (end == file_end) { // end tag not found
            _message_handler("addDynamicLibrary: filename=" + filename + " end tag was missing");
            break;
        }

        // check if there is a start tag before the end search the next start tag and check if it is interleaved with current section
        if (getFirstFromPair(search_start(start + 1, end)) < end) {
            _message_handler("addDynamicLibrary: filename=" + filename + " found a start tag before the expected end tag");
            continue;
        }

        auto key_value = parseKeyValue(filename, start, end);

        if (key_value.empty())
            continue; // empty or invalid export

        key_value["library_filename"] = file_path;

        auto ext = parse(filename, std::move(key_value));

        if (ext.isValid())
            info.extensions.push_back(std::move(ext));
    }

    // still possible if the file has an invalid start tag
    const auto count = info.extensions.size();

    if (count == 0)
        return 0;

    _known_extensions[file_path] = std::move(info);

    return count;
}

std::unordered_map<std::string, std::string> ExtensionSystem::parseKeyValue(const std::string& filename, const char* start, const char* end) {
    std::unordered_map<std::string, std::string> result;

    bool successful = split({start, end - 1}, '\0', [&](const std::string& iter) {
        std::size_t pos = iter.find('=');
        if (pos == std::string::npos) {
            _message_handler("addDynamicLibrary: filename=" + filename + " '=' is missing (" + iter + "), ignore extension export"); // NOLINT
            return false;
        }
        const auto key   = iter.substr(0, pos);
        const auto value = iter.substr(pos + 1);
        if (result.find(key) != result.end()) {
            _message_handler("addDynamicLibrary: filename=" + filename + " duplicate key (" + key + ") found, ignore extension export"); // NOLINT
            return false;
        }
        result[key] = value;
        return true;
    });

    if (successful && result.empty())
        _message_handler("addDynamicLibrary: filename=" + filename + " metadata description didn't contain any data, ignore it");

    return result;
}

ExtensionDescription ExtensionSystem::parse(const std::string& filename, std::unordered_map<std::string, std::string>&& desc) {
    if (_verify_compiler
        && (desc[desc_start] != EXTENSION_SYSTEM_EXTENSION_API_VERSION_STR || desc["compiler"] != EXTENSION_SYSTEM_COMPILER
            || desc["compiler_version"] != EXTENSION_SYSTEM_COMPILER_VERSION_STR || desc["build_type"] != EXTENSION_SYSTEM_BUILD_TYPE)) {
        // clang-format off
            _message_handler("addDynamicLibrary: Ignore file " + filename + ". Compilation options didn't match or were invalid ("
                               "version="           + desc[desc_start]
                             + " compiler="         + desc["compiler"]
                             + " compiler_version=" + desc["compiler_version"]
                             + " build_type="       + desc["build_type"]
                             + " expected version=" EXTENSION_SYSTEM_EXTENSION_API_VERSION_STR
                             " compiler="           EXTENSION_SYSTEM_COMPILER
                             " compiler_version="   EXTENSION_SYSTEM_COMPILER_VERSION_STR
                             " build_type="         EXTENSION_SYSTEM_BUILD_TYPE
                             ")");
        // clang-format on
        return {};
    }

    std::string name;

    auto is_invalid = [&](const std::string& str) {
        auto iter = desc.find(str);
        if (iter == desc.end()) {
            _message_handler("addDynamicLibrary: filename=" + filename + " " + name + str + " has to be set"); // NOLINT
            return true;
        }

        if (iter->second.empty()) {
            _message_handler("addDynamicLibrary: filename=" + filename + " " + name + str + " can not be empty"); // NOLINT
            return true;
        }

        return false;
    };

    if (is_invalid("name"))
        return {};

    name = "name= " + desc.at("name");

    if (is_invalid("interface_name"))
        return {};

    if (is_invalid("entry_point"))
        return {};

    if (is_invalid("version"))
        return {};

    ExtensionVersion version{};

    std::stringstream str{desc.at("version")};
    str >> version;

    if (str.fail()) {
        _message_handler("addDynamicLibrary: filename=" + filename + " " + name + " couldn't parse version"); // NOLINT
        return {};
    }

    desc.erase(desc_start);

    return ExtensionDescription{std::move(desc), version};
}

void ExtensionSystem::removeDynamicLibrary(const std::string& filename) {
    auto real_filename = getRealFilename(filename);
    auto iter          = _known_extensions.find(real_filename);
    if (iter != _known_extensions.end())
        _known_extensions.erase(iter);
}

void ExtensionSystem::searchDirectory(const std::string& path, bool recursive) {
    debugMessage("search directory path=" + path + " recursive=" + (recursive ? "true" : "false"));
    std::vector<char> buffer;
    filesystem::forEachFileInDirectory(path,
                                       [this, &buffer](const filesystem::path& p) {
                                           if (p.extension().string() == DynamicLibrary::fileExtension())
                                               addDynamicLibrary(p.string(), buffer);
                                           else
                                               debugMessage("ignore file " + p.string() + " due to wrong fileExtension ("
                                                            + DynamicLibrary::fileExtension() + ")");
                                       },
                                       recursive);
}

void ExtensionSystem::searchDirectory(const std::string& path, const std::string& required_prefix, bool recursive) {
    debugMessage("search directory path=" + path + "required_prefix=" + required_prefix + " recursive=" + (recursive ? "true" : "false"));
    std::vector<char> buffer;
    const std::size_t required_prefix_length = required_prefix.length();
    filesystem::forEachFileInDirectory(path,
                                       [this, &buffer, required_prefix_length, &required_prefix](const filesystem::path& p) {
                                           if (p.extension().string() == DynamicLibrary::fileExtension()
                                               && p.filename().string().compare(0, required_prefix_length, required_prefix) == 0)
                                               addDynamicLibrary(p.string(), buffer);
                                           else
                                               debugMessage("ignore file " + p.string() + " either due to wrong required_prefix or wrong fileExtension ("
                                                            + p.extension().string() + ")");
                                       },
                                       recursive);
}

std::vector<ExtensionDescription> ExtensionSystem::extensions(const std::vector<std::pair<std::string, std::string>>& metaDataFilter) const {
    std::unordered_map<std::string, std::unordered_set<std::string>> filter_map;

    for (const auto& f : metaDataFilter)
        filter_map[f.first].insert(f.second);

    std::vector<ExtensionDescription> result;

    for (const auto& i : _known_extensions) {
        for (const auto& j : i.second.extensions) {
            // check all filters
            bool add_extension = true;

            for (const auto& filter : filter_map) {
                auto extended = j.data();

                // search extended data if filtered metadata is present
                auto ext_iter = extended.find(filter.first);

                if (ext_iter == extended.end()) {
                    add_extension = false;
                    break;
                }

                // check if metadata value is within filter values
                if (filter.second.find(ext_iter->second) == filter.second.end()) {
                    add_extension = false;
                    break;
                }
            }
            if (add_extension)
                result.push_back(j);
        }
    }

    return result;
}

std::vector<ExtensionDescription> ExtensionSystem::extensions() const {
    std::vector<ExtensionDescription> list;

    for (const auto& i : _known_extensions)
        for (const auto& j : i.second.extensions)
            list.push_back(j);

    return list;
}

ExtensionDescription ExtensionSystem::findDescription(const std::string& interface_name, const std::string& name, ExtensionVersion version) const {
    for (const auto& i : _known_extensions)
        for (const auto& j : i.second.extensions)
            if (j.interface_name() == interface_name && j.name() == name && j.version() == version)
                return j;

    return {};
}

ExtensionDescription ExtensionSystem::findDescription(const std::string& interface_name, const std::string& name) const {
    ExtensionVersion            highest_version{};
    const ExtensionDescription* desc{};

    for (const auto& i : _known_extensions) {
        for (const auto& j : i.second.extensions) {
            if (j.interface_name() == interface_name && j.name() == name && j.version() > highest_version) {
                highest_version = j.version();
                desc            = &j;
            }
        }
    }

    if (desc == nullptr)
        return {};

    return *desc;
}

void ExtensionSystem::debugMessage(const std::string& msg) {
    if (_debug_output)
        _message_handler(msg);
}

void ExtensionSystem::setVerifyCompiler(bool enable) {
    _verify_compiler = enable;
}

void ExtensionSystem::setEnableDebugOutput(bool enable) {
    _debug_output = enable;
}
