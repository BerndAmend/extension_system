/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014
		Distributed under the Boost Software License, Version 1.0.
		(See accompanying file LICENSE_1_0.txt or copy at
		http://www.boost.org/LICENSE_1_0.txt)
*/
#include <extension_system/ExtensionSystem.hpp>

#include <fstream>
#include <algorithm>
#include <unordered_set>
#include <extension_system/string.hpp>
#include <extension_system/filesystem.hpp>
#include <iostream>

#ifdef EXTENSION_SYSTEM_OS_LINUX
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>

	class MemoryMap {
	public:
		MemoryMap() {}
		MemoryMap(const MemoryMap&) =delete;
		MemoryMap& operator=(const MemoryMap&) =delete;
		~MemoryMap() {
			close();
		}

		bool open(const std::string &filename) {
			struct stat sb;
			int fd = ::open(filename.c_str(), O_RDONLY);

			if(fd == -1)
				return false;

			fstat(fd, &sb);

			_size = sb.st_size;

			int flags =  MAP_FILE | MAP_PRIVATE;

			#ifdef MAP_POPULATE
			flags |= MAP_POPULATE;
			#endif

			char *addr = reinterpret_cast<char *>(mmap(NULL, _size, PROT_READ, flags, fd, 0));

			::close(fd);

			if (addr == MAP_FAILED)
				return false;

			_data = addr;
			return true;
		}

		void close() {
			if(_data != nullptr) {
				munmap(_data,_size);
				_data = nullptr;
				_size = 0;
			}
		}

		char *data() { return _data; }

		std::size_t size() const { return _size; }

	private:
		char *_data = nullptr;
		std::size_t _size = 0;
	};
#else
	class MemoryMap {
	public:
		MemoryMap() {}
		~MemoryMap() {
			close();
		}

		bool open(const std::string &) {
			return false;
		}

		void close() {}

		char *data() { return _data; }

		std::size_t size() const { return _size; }

	private:
		char *_data = nullptr;
		std::size_t _size = 0;
	};
#endif

using namespace extension_system;

static std::string getRealFilename(const std::string &filename) {
	filesystem::path filen(filename);

	if (!filesystem::exists(filen)) { // check if the file exists
		if(filesystem::exists(filesystem::path(filename + DynamicLibrary::fileExtension()))) {
			filen = filename + DynamicLibrary::fileExtension();
		} else {
			return "";
		}
	}

	return canonical(filen).generic_string();
}

static std::size_t find_string(const char *data, std::size_t data_size, const std::string &search_string, std::size_t start_pos) {
	// HINT: memmem was much slower than std::search
	const char *found = std::search(data+start_pos, data+data_size, search_string.c_str(), search_string.c_str()+search_string.length());
	if(found == data+data_size)
		return std::string::npos;
	else
		return static_cast<std::size_t>(found-data);
}

ExtensionSystem::ExtensionSystem()
	: _message_handler([](const std::string &msg) { std::cerr << "ExtensionSystem::" << msg << std::endl;}), _extension_system_alive(std::make_shared<bool>(true)) {}

bool ExtensionSystem::addDynamicLibrary(const std::string &filename) {
	std::unique_lock<std::mutex> lock(_mutex);
	std::vector<char> buffer;
	return _addDynamicLibrary(filename, buffer);
}

bool ExtensionSystem::_addDynamicLibrary(const std::string &filename, std::vector<char> &buffer) {
	std::string filePath = getRealFilename(filename);

	if(filePath.empty()) {
		_message_handler("addDynamicLibrary: neither " + filename + " nor " + filename + DynamicLibrary::fileExtension() + " exist.");
		return false;
	}

	if(filesystem::is_directory(filePath)) {
		_message_handler("addDynamicLibrary: doesn't support adding directories directory=" + filename);
		return false;
	}

	auto already_loaded = _known_extensions.find(filePath);

	// don't reload library, if there are already references to one contained extension
	if( already_loaded!=_known_extensions.end() && !already_loaded->second.dynamic_library.expired() ) {
		return false;
	}

	std::size_t file_length = 0;
	char *file_content = nullptr;

	std::ifstream file;
	MemoryMap memoryMap;

	if(memoryMap.open(filePath)) {
		file_length = memoryMap.size();
		file_content = memoryMap.data();
	} else {
		file.open(filePath, std::ios::in | std::ios::binary | std::ios::ate);

		if(!file) {
			_message_handler("couldn't open file");
			return false;
		}

		file_length = file.tellg();

		if(file_length <= 0) {
			_message_handler("invalid file size");
			return false;
		}

		file.seekg(0, std::ios::beg);

		if(buffer.size() < static_cast<unsigned int>(file_length))
			buffer.resize(static_cast<unsigned int>(file_length));

		file_content = buffer.data();

		file.read(file_content, file_length);
		file.close();
	}

	std::size_t found = find_string(file_content, file_length, desc_start, 0);

	if(found == std::string::npos) { // no tag found, skip file
		if(_check_for_upx_compression) {
			// check only for upx if the file didn't contain any tags at all
			const std::size_t found_upx_exclamation_mark = find_string(file_content, file_length, upx_exclamation_mark_string, 0);
			const std::size_t found_upx = find_string(file_content, file_length, upx_string, 0);

			if(found_upx != std::string::npos &&
					found_upx_exclamation_mark != std::string::npos &&
					found_upx < found_upx_exclamation_mark ) {
				_message_handler("addDynamicLibrary: Couldn't find any extensions in file " + filename + ", it seems the file is compressed using upx. ");
			}
		}
		return false;
	}

	std::vector<std::unordered_map<std::string, std::string> > data;

	while(found != std::string::npos) {
		std::size_t start = found;

		// search the end tag
		found = find_string(file_content, file_length, desc_end, found+1);
		std::size_t end = found;

		if(end == std::string::npos) { // end tag not found
			_message_handler("addDynamicLibrary: filename=" + filename + " end tag was missing");
			break;
		}

		// search the next start tag and check if it is interleaved with current section
		found = find_string(file_content, file_length, desc_start, start+1);

		if(found < end ) {
			_message_handler("addDynamicLibrary: filename=" + filename + " found a start tag before the expected end tag");
			continue;
		}

		const std::string raw = std::string(file_content+start, end-start-1);

		bool failed = false;
		std::unordered_map<std::string, std::string> result;
		std::string delimiter;
		delimiter += '\0';
		split(raw, delimiter, [&](const std::string &iter) {
			std::size_t pos = iter.find('=');
			if(pos == std::string::npos) {
				_message_handler("addDynamicLibrary: filename=" + filename + " '=' is missing (" + iter +"). Ignore entry.");
				failed = true;
				return false;
			}
			const auto key = iter.substr(0, pos);
			const auto value = iter.substr(pos+1);
			if(result.find(key) != result.end()) {
				_message_handler("addDynamicLibrary: filename=" + filename + " duplicate key (" + key + ") found. Ignore entry");
				failed = true;
				return false;
			}
			result[key] = value;
			return true;
		});

		result["library_filename"] = filePath;

		if(failed) {
			// we already printed an error
		} else if(!result.empty()) {
			data.push_back(result);
		} else {
			_message_handler("addDynamicLibrary: filename=" + filename + " metadata description didn't contain any data, ignore it");
		}

		found = find_string(file_content, file_length, desc_start, end);
	}

	std::vector<ExtensionDescription> extension_list;
	for(auto &iter : data) {
		ExtensionDescription desc(iter);
		if(		!_verify_compiler  ||
				(desc[desc_start] == EXTENSION_SYSTEM_STR(EXTENSION_SYSTEM_EXTENSION_API_VERSION)
#if defined(EXTENSION_SYSTEM_COMPILER_GPLUSPLUS) || defined(EXTENSION_SYSTEM_COMPILER_CLANG)
				&& (desc["compiler"] == EXTENSION_SYSTEM_COMPILER_STR_CLANG || desc["compiler"] == EXTENSION_SYSTEM_COMPILER_STR_GPLUSPLUS)
#else
				&& desc["compiler"] == EXTENSION_SYSTEM_COMPILER
				&& desc["compiler_version"] == EXTENSION_SYSTEM_COMPILER_VERSION_STR
				&& desc["build_type"] == EXTENSION_SYSTEM_BUILD_TYPE
#endif
				// TODO: check operating system
				)) {

			desc._data.erase(desc_start);

			if(desc.name().empty()) {
				_message_handler("addDynamicLibrary: filename=" + filename +" name was empty or not set");
				continue;
			}

			if(desc.interface_name().empty()) {
				_message_handler("addDynamicLibrary: filename=" + filename + " name=" + desc.name() + " interface_name was empty or not set");
				continue;
			}

			if(desc.entry_point().empty()) {
				_message_handler("addDynamicLibrary: filename=" + filename + " name=" + desc.name() + " entry_point was empty or not set");
				continue;
			}

			if(desc.version() == 0) {
				_message_handler("addDynamicLibrary: filename=" + filename + " name=" + desc.name() + ": version number was invalid or 0");
				continue;
			}

			extension_list.push_back(desc);
		} else {
			_message_handler("addDynamicLibrary: Ignore file "+
							filename + ". Compilation options didn't match or were invalid " +
							"(version=" + desc[desc_start] +
							" compiler=" + desc["compiler"] +
							" compiler_version=" + desc["compiler_version"] +
							" build_type=" + desc["build_type"] +
							" expected " +
							" version=" +EXTENSION_SYSTEM_STR(EXTENSION_SYSTEM_EXTENSION_API_VERSION) +
							" compiler=" + EXTENSION_SYSTEM_COMPILER +
							" compiler_version=" + EXTENSION_SYSTEM_COMPILER_VERSION_STR +
							" build_type=" + EXTENSION_SYSTEM_BUILD_TYPE +
							")");
		}
	}

	if(!extension_list.empty()) {
		//TODO: handling of extensions with same name and version number
		_known_extensions[filePath] = LibraryInfo(extension_list);
		return true;
	} else {
		// still possible if the file has an invalid start tag
		return false;
	}
}

void ExtensionSystem::removeDynamicLibrary(const std::string &filename)
{
	std::unique_lock<std::mutex> lock(_mutex);
	auto real_filename = getRealFilename(filename);
	auto iter = _known_extensions.find(real_filename);
	if(iter != _known_extensions.end())
		_known_extensions.erase(iter);
}

void ExtensionSystem::searchDirectory( const std::string& path) {
	std::vector<char> buffer;
	std::unique_lock<std::mutex> lock(_mutex);
	filesystem::forEachFileInDirectory(path, [this, &buffer](const filesystem::path &p){
		if (p.extension().string() == DynamicLibrary::fileExtension())
			_addDynamicLibrary(p.string(), buffer);
	});
}

void ExtensionSystem::searchDirectory( const std::string& path, const std::string &required_prefix) {
	std::vector<char> buffer;
	std::unique_lock<std::mutex> lock(_mutex);
	const std::size_t required_prefix_length = required_prefix.length();
	filesystem::forEachFileInDirectory(path, [this, &buffer, required_prefix_length, &required_prefix](const filesystem::path &p){
		if (p.extension().string() == DynamicLibrary::fileExtension() &&
				p.filename().string().compare(0, required_prefix_length, required_prefix)==0)
			_addDynamicLibrary(p.string(), buffer);
	});
}

std::vector<ExtensionDescription> ExtensionSystem::extensions(const std::vector<std::pair<std::string, std::string> > &metaDataFilter) const
{
	std::unordered_map<std::string, std::unordered_set<std::string> > filterMap;

	for(const auto &f : metaDataFilter )
		filterMap[f.first].insert(f.second);

	std::unique_lock<std::mutex> lock(_mutex);
	std::vector<ExtensionDescription> result;

	for(const auto &i : _known_extensions)
		for(const auto &j : i.second.extensions) {
			// check all filters
			bool addExtension = true;

			for(const auto &filter : filterMap ) {
				auto extended = j._data;

				// search extended data if filtered metadata is present
				auto extIter = extended.find(filter.first);

				if( extIter == extended.end() ) {
					addExtension = false;
					break;
				}

				// check if metadata value is within filter values
				if( filter.second.find(extIter->second) == filter.second.end() ) {
					addExtension = false;
					break;
				}
			}
			if( addExtension )
				result.push_back(j);
		}
	return result;

}

std::vector<ExtensionDescription> ExtensionSystem::extensions() const {
	std::unique_lock<std::mutex> lock(_mutex);
	std::vector<ExtensionDescription> list;

	for(const auto &i : _known_extensions)
		for(const auto &j : i.second.extensions)
			list.push_back(j);

	return list;
}

ExtensionDescription ExtensionSystem::_findDescription(const std::string& interface_name, const std::string& name, unsigned int version) const {
	for(const auto &i : _known_extensions)
		for(const auto &j : i.second.extensions)
			if(j.interface_name() == interface_name && j.name() == name && j.version() == version)
				return j;

	return ExtensionDescription();
}

ExtensionDescription ExtensionSystem::_findDescription(const std::string& interface_name, const std::string& name) const {
	unsigned int highest_version = 0;
	const ExtensionDescription *desc = nullptr;

	for(const auto &i : _known_extensions) {
		for(const auto &j : i.second.extensions) {
			if(j.interface_name() == interface_name && j.name() == name && j.version() > highest_version ) {
				highest_version = j.version();
				desc = &j;
			}
		}
	}

	return desc ? *desc : ExtensionDescription();
}


void ExtensionSystem::setVerifyCompiler(bool enable) {
	_verify_compiler = enable;
}
