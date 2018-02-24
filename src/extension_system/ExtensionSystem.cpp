/**
    @file
    @copyright
        Copyright Bernd Amend and Michael Adam 2014-2018
        Distributed under the Boost Software License, Version 1.0.
        (See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt)
*/
#include <extension_system/ExtensionSystem.hpp>

#include <algorithm>
#include <extension_system/filesystem.hpp>
#include <extension_system/string.hpp>
#include <iostream>
#include <unordered_set>

#ifdef EXTENSION_SYSTEM_USE_BOOST
#define BOOST_DATE_TIME_NO_LIB
#include <boost/algorithm/searching/boyer_moore.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#else
#include <fstream>
#endif

using namespace extension_system;

namespace {
inline std::string getRealFilename(const std::string& filename) {
    filesystem::path filen(filename);

    if (!filesystem::exists(filen)) { // check if the file exists
        if (filesystem::exists(filesystem::path(filename + DynamicLibrary::fileExtension()))) {
            filen = filename + DynamicLibrary::fileExtension();
        } else {
            return "";
        }
    }

    return canonical(filen).generic_string();
}

#ifdef EXTENSION_SYSTEM_USE_BOOST
using StringSearch = boost::algorithm::boyer_moore<const char*>;
#else
class StringSearch {
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
    : _message_handler([](const std::string& msg) { std::cerr << "ExtensionSystem::" << msg << std::endl; })
    , _extension_system_alive(std::make_shared<bool>(true)) {}

bool ExtensionSystem::addDynamicLibrary(const std::string& filename) {
    std::unique_lock<std::mutex> lock(_mutex);
    std::vector<char>            buffer;
    return addDynamicLibrary(filename, buffer);
}

bool ExtensionSystem::addDynamicLibrary(const std::string& filename, std::vector<char>& buffer) {
    debugMessage("check file " + filename);
    const std::string file_path = getRealFilename(filename);

    if (file_path.empty()) {
        _message_handler("addDynamicLibrary: neither " + filename + " nor " + filename + DynamicLibrary::fileExtension() + " exist.");
        return false;
    }

    if (filesystem::is_directory(file_path)) {
        _message_handler("addDynamicLibrary: doesn't support adding directories directory=" + filename);
        return false;
    }

    auto already_loaded = _known_extensions.find(file_path);

    // don't reload library, if there are already references to one contained extension
    if (already_loaded != _known_extensions.end() && !already_loaded->second.dynamic_library.expired()) {
        return false;
    }

    std::size_t file_length  = 0;
    const char* file_content = nullptr;

#ifdef EXTENSION_SYSTEM_USE_BOOST
    (void)buffer;
    const boost::interprocess::mode_t mode = boost::interprocess::read_only;
    boost::interprocess::file_mapping fm;

    boost::interprocess::mapped_region file;

    try {
        fm = boost::interprocess::file_mapping(file_path.c_str(), mode);
    } catch (const boost::interprocess::interprocess_exception& e) {
        _message_handler(std::string("file_mapping failed ") + e.what());
        return false;
    }

    try {
        file = boost::interprocess::mapped_region(fm, mode, 0, 0);
    } catch (const boost::interprocess::interprocess_exception& e) {
        _message_handler(std::string("mapped_region failed ") + e.what());
        return false;
    }

    file_length  = file.get_size();
    file_content = reinterpret_cast<const char*>(file.get_address()); // NOLINT
#else
    {
        std::ifstream file;
        file.open(file_path, std::ios::in | std::ios::binary | std::ios::ate);

        if (!file) {
            _message_handler("couldn't open file");
            return false;
        }

        file_length = static_cast<std::size_t>(file.tellg());

        if (file_length <= 0) {
            _message_handler("invalid file size");
            return false;
        }

        file.seekg(0, std::ios::beg);

        if (buffer.size() < static_cast<unsigned int>(file_length)) {
            buffer.resize(static_cast<unsigned int>(file_length));
        }

        file_content = buffer.data();

        file.read(buffer.data(), static_cast<std::streamsize>(file_length));
    }
#endif

    const char* file_end = file_content + file_length;

    const std::string start_string = desc_start;
    StringSearch      search_start(start_string.c_str(), start_string.c_str() + start_string.length());

    const char* found = getFirstFromPair(search_start(file_content, file_end));

    if (found == file_end) { // no tag found, skip file
        if (_check_for_upx_compression) {
            StringSearch search_upx(upx_string.c_str(), upx_string.c_str() + upx_string.length());
            StringSearch search_upx_exclamation_mark(upx_exclamation_mark_string.c_str(),
                                                     upx_exclamation_mark_string.c_str() + upx_exclamation_mark_string.length());
            // check only for upx if the file didn't contain any tags at all
            const char* found_upx_exclamation_mark = getFirstFromPair(search_upx_exclamation_mark(file_content, file_end));
            const char* found_upx                  = getFirstFromPair(search_upx(file_content, file_end));

            if (found_upx != file_end && found_upx_exclamation_mark != file_end && found_upx < found_upx_exclamation_mark) {
                _message_handler("addDynamicLibrary: Couldn't find any extensions in file " + filename
                                 + ", it seems the file is compressed using upx. ");
            }
        }
        return false;
    }

    const std::string end_string = desc_end;
    StringSearch      search_end(end_string.c_str(), end_string.c_str() + end_string.length());

    std::vector<std::unordered_map<std::string, std::string>> data;

    while (found != file_end) {
        const char* start = found;

        // search the end tag
        found           = getFirstFromPair(search_end(found + 1, file_end));
        const char* end = found;

        if (end == file_end) { // end tag not found
            _message_handler("addDynamicLibrary: filename=" + filename + " end tag was missing");
            break;
        }

        // search the next start tag and check if it is interleaved with current section
        found = getFirstFromPair(search_start(start + 1, file_end));

        if (found < end) {
            _message_handler("addDynamicLibrary: filename=" + filename + " found a start tag before the expected end tag");
            continue;
        }

        const std::string raw = std::string(start, static_cast<std::size_t>(end - start - 1));

        bool                                         failed = false;
        std::unordered_map<std::string, std::string> result;
        std::string                                  delimiter;
        delimiter += '\0';
        split(raw, delimiter, [&](const std::string& iter) {
            std::size_t pos = iter.find('=');
            if (pos == std::string::npos) {
                _message_handler("addDynamicLibrary: filename=" + filename + " '=' is missing (" + iter + "). Ignore entry."); // NOLINT
                failed = true;
                return false;
            }
            const auto key   = iter.substr(0, pos);
            const auto value = iter.substr(pos + 1);
            if (result.find(key) != result.end()) {
                _message_handler("addDynamicLibrary: filename=" + filename + " duplicate key (" + key + ") found. Ignore entry"); // NOLINT
                failed = true;
                return false;
            }
            result[key] = value;
            return true;
        });

        result["library_filename"] = file_path;

        if (failed) {
            // we already printed an error
        } else if (!result.empty()) {
            data.push_back(result);
        } else {
            _message_handler("addDynamicLibrary: filename=" + filename + " metadata description didn't contain any data, ignore it");
        }

        found = getFirstFromPair(search_start(end, file_end));
    }

    std::vector<ExtensionDescription> extension_list;
    for (const auto& iter : data) {
        ExtensionDescription desc(iter);
        if (!_verify_compiler
            || (desc[desc_start] == EXTENSION_SYSTEM_STR(EXTENSION_SYSTEM_EXTENSION_API_VERSION)
#if defined(EXTENSION_SYSTEM_COMPILER_GPLUSPLUS) || defined(EXTENSION_SYSTEM_COMPILER_CLANG)
                && (desc["compiler"] == EXTENSION_SYSTEM_COMPILER_STR_CLANG || desc["compiler"] == EXTENSION_SYSTEM_COMPILER_STR_GPLUSPLUS)
#else
                && desc["compiler"] == EXTENSION_SYSTEM_COMPILER && desc["compiler_version"] == EXTENSION_SYSTEM_COMPILER_VERSION_STR
                && desc["build_type"] == EXTENSION_SYSTEM_BUILD_TYPE
#endif
                // Should we check the operating system too?
                )) {

            desc._data.erase(desc_start);

            if (desc.name().empty()) {
                _message_handler("addDynamicLibrary: filename=" + filename + " name was empty or not set");
                continue;
            }

            if (desc.interface_name().empty()) {
                _message_handler("addDynamicLibrary: filename=" + filename + " name=" + desc.name() + " interface_name was empty or not set");
                continue;
            }

            if (desc.entry_point().empty()) {
                _message_handler("addDynamicLibrary: filename=" + filename + " name=" + desc.name() + " entry_point was empty or not set");
                continue;
            }

            if (desc.version() == 0) {
                _message_handler("addDynamicLibrary: filename=" + filename + " name=" + desc.name() + ": version number was invalid or 0");
                continue;
            }

            extension_list.push_back(desc);
        } else {
            // clang-format off
            _message_handler("addDynamicLibrary: Ignore file " + filename + ". Compilation options didn't match or were invalid ("
                             + "version="          + desc[desc_start]
                             + " compiler="         + desc["compiler"]
                             + " compiler_version=" + desc["compiler_version"]
                             + " build_type="       + desc["build_type"]
                             + " expected version=" EXTENSION_SYSTEM_STR(EXTENSION_SYSTEM_EXTENSION_API_VERSION)
                             " compiler="           EXTENSION_SYSTEM_COMPILER
                             " compiler_version="   EXTENSION_SYSTEM_COMPILER_VERSION_STR
                             " build_type="         EXTENSION_SYSTEM_BUILD_TYPE
                             ")");
            // clang-format on
        }
    }

    if (extension_list.empty()) {
        // still possible if the file has an invalid start tag
        return false;
    }

    // The handling of extensions with same name and version number is currently broken
    _known_extensions[file_path] = LibraryInfo(extension_list);
    return true;
}

void ExtensionSystem::removeDynamicLibrary(const std::string& filename) {
    std::unique_lock<std::mutex> lock(_mutex);
    auto                         real_filename = getRealFilename(filename);
    auto                         iter          = _known_extensions.find(real_filename);
    if (iter != _known_extensions.end()) {
        _known_extensions.erase(iter);
    }
}

void ExtensionSystem::searchDirectory(const std::string& path, bool recursive) {
    debugMessage("search directory path=" + path + " recursive=" + (recursive ? "true" : "false"));
    std::vector<char>            buffer;
    std::unique_lock<std::mutex> lock(_mutex);
    filesystem::forEachFileInDirectory(path,
                                       [this, &buffer](const filesystem::path& p) {
                                           if (p.extension().string() == DynamicLibrary::fileExtension()) {
                                               addDynamicLibrary(p.string(), buffer);
                                           } else {
                                               debugMessage("ignore file " + p.string() + " due to wrong fileExtension ("
                                                            + DynamicLibrary::fileExtension() + ")");
                                           }
                                       },
                                       recursive);
}

void ExtensionSystem::searchDirectory(const std::string& path, const std::string& required_prefix, bool recursive) {
    debugMessage("search directory path=" + path + "required_prefix=" + required_prefix + " recursive=" + (recursive ? "true" : "false"));
    std::vector<char>            buffer;
    std::unique_lock<std::mutex> lock(_mutex);
    const std::size_t            required_prefix_length = required_prefix.length();
    filesystem::forEachFileInDirectory(path,
                                       [this, &buffer, required_prefix_length, &required_prefix](const filesystem::path& p) {
                                           if (p.extension().string() == DynamicLibrary::fileExtension()
                                               && p.filename().string().compare(0, required_prefix_length, required_prefix) == 0) {
                                               addDynamicLibrary(p.string(), buffer);
                                           } else {
                                               debugMessage("ignore file " + p.string() + " either due to wrong required_prefix or wrong fileExtension ("
                                                            + p.extension().string() + ")");
                                           }
                                       },
                                       recursive);
}

std::vector<ExtensionDescription> ExtensionSystem::extensions(const std::vector<std::pair<std::string, std::string>>& metaDataFilter) const {
    std::unordered_map<std::string, std::unordered_set<std::string>> filter_map;

    for (const auto& f : metaDataFilter) {
        filter_map[f.first].insert(f.second);
    }

    std::unique_lock<std::mutex>      lock(_mutex);
    std::vector<ExtensionDescription> result;

    for (const auto& i : _known_extensions) {
        for (const auto& j : i.second.extensions) {
            // check all filters
            bool add_extension = true;

            for (const auto& filter : filter_map) {
                auto extended = j._data;

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
            if (add_extension) {
                result.push_back(j);
            }
        }
    }

    return result;
}

std::vector<ExtensionDescription> ExtensionSystem::extensions() const {
    std::unique_lock<std::mutex>      lock(_mutex);
    std::vector<ExtensionDescription> list;

    for (const auto& i : _known_extensions) {
        for (const auto& j : i.second.extensions) {
            list.push_back(j);
        }
    }

    return list;
}

ExtensionDescription ExtensionSystem::findDescriptionUnsafe(const std::string& interface_name, const std::string& name, unsigned int version) const {
    for (const auto& i : _known_extensions) {
        for (const auto& j : i.second.extensions) {
            if (j.interface_name() == interface_name && j.name() == name && j.version() == version) {
                return j;
            }
        }
    }

    return ExtensionDescription();
}

ExtensionDescription ExtensionSystem::findDescriptionUnsafe(const std::string& interface_name, const std::string& name) const {
    unsigned int                highest_version = 0;
    const ExtensionDescription* desc            = nullptr;

    for (const auto& i : _known_extensions) {
        for (const auto& j : i.second.extensions) {
            if (j.interface_name() == interface_name && j.name() == name && j.version() > highest_version) {
                highest_version = j.version();
                desc            = &j;
            }
        }
    }

    return desc != nullptr ? *desc : ExtensionDescription();
}

void ExtensionSystem::debugMessage(const std::string& msg) {
    if (_debug_output) {
        _message_handler(msg);
    }
}

void ExtensionSystem::setVerifyCompiler(bool enable) {
    _verify_compiler = enable;
}

void ExtensionSystem::setEnableDebugOutput(bool enable) {
    _debug_output = enable;
}
