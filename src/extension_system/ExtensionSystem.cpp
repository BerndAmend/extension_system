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

ExtensionSystem::ExtensionSystem()
	: _verify_compiler(true), _message_handler([](const std::string &msg) { std::cerr << msg << std::endl;}), _extension_system_alive(std::make_shared<bool>(true)) {}

bool ExtensionSystem::addDynamicLibrary(const std::string &filename) {
	std::unique_lock<std::mutex> lock(_mutex);

	std::string filePath = getRealFilename(filename);

	if(filePath.empty()) {
		_message_handler("ExtensionSystem addDynamicLibrary neither " + filename + " nor " + filename + DynamicLibrary::fileExtension() + " exist.");
		return false;
	}

	if(filesystem::is_directory(filePath)) {
		_message_handler("ExtensionSystem addDynamicLibrary doesn't support adding directories directory=" + filename);
		return false;
	}

	auto already_loaded = _knownExtensions.find(filePath);

	// don't reload library, if there are already references to one contained extension
	if( already_loaded!=_knownExtensions.end() && !already_loaded->second.dynamicLibrary.expired() ) {
		return false;
	}

	std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);

	std::ifstream::pos_type file_length = file.tellg();

	if(file_length <= 0) {
		_message_handler("invalid file size");
		return false;
	}

	file.seekg(0, std::ios::beg);

	std::vector<char> str(static_cast<unsigned int>(file_length));
	file.read(str.data(), file_length);
	file.close();

	std::string t(str.data(), str.size());

	std::size_t found = t.find(desc_start);

	std::size_t first_end = t.find(desc_end);

	if(first_end < found) {
		_message_handler("ExtensionSystem filename=" + filename + " the first end tag appears before the first start tag");
	}

	std::vector<std::unordered_map<std::string, std::string> > data;

	while(found != std::string::npos) {
		std::size_t start = found;

		// search the end tag
		found=t.find(desc_end, found+1);
		std::size_t end = found;

		if(end == std::string::npos) { // end tag not found
			_message_handler("ExtensionSystem filename=" + filename + " end tag was missing");
			break;
		}

		// search the next start tag and check if it is interleaved with current section
		found=t.find(desc_start, start+1);

		if(found < end ) {
			_message_handler("ExtensionSystem filename=" + filename + " found a start tag before the expected end tag");
			continue;
		}

		const std::string raw = std::string(str.data()+start, end-start-1);

		bool failed = false;
		std::unordered_map<std::string, std::string> result;
		std::string delimiter;
		delimiter += '\0';
		split(raw, delimiter, [&](const std::string &iter) {
			std::size_t pos = iter.find('=');
			if(pos == std::string::npos) {
				_message_handler("ExtensionSystem filename=" + filename + " '=' is missing (" + iter +"). Ignore entry.");
				failed = true;
				return false;
			}
			const auto key = iter.substr(0, pos);
			const auto value = iter.substr(pos+1);
			if(result.find(key) != result.end()) {
				_message_handler("ExtensionSystem filename=" + filename + " duplicate key (" + key + ") found. Ignore entry");
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
			_message_handler("ExtensionSystem filename=" + filename + " metadata description didn't contain any data, ignore it");
		}

		found=t.find(desc_start, end);
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
				_message_handler("ExtensionSystem filename=" + filename +" name was empty or not set");
				continue;
			}

			if(desc.interface_name().empty()) {
				_message_handler("ExtensionSystem filename=" + filename + " name=" + desc.name() + " interface_name was empty or not set");
				continue;
			}

			if(desc.entry_point().empty()) {
				_message_handler("ExtensionSystem filename=" + filename + " name=" + desc.name() + " entry_point was empty or not set");
				continue;
			}

			if(desc.version() == 0) {
				_message_handler("ExtensionSystem filename=" + filename + " name=" + desc.name() + ": version number was invalid or 0");
				continue;
			}

			extension_list.push_back(desc);
		} else {
			_message_handler("ExtensionSystem: Ignore file "+
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
		_knownExtensions[filePath] = LibraryInfo(extension_list);
		return true;
	} else {
		const std::size_t found_upx_exclamation_mark = t.find(upx_exclamation_mark_string);
		const std::size_t found_upx = t.find(upx_string);

		if(found_upx != std::string::npos &&
				found_upx_exclamation_mark != std::string::npos &&
				found_upx < found_upx_exclamation_mark ) {
			_message_handler("ExtensionSystem: Couldn't find any extensions in file " + filename + ", it seems the file is compressed using upx. ");
		}

		return false;
	}
}

void ExtensionSystem::removeDynamicLibrary(const std::string &filename)
{
	std::unique_lock<std::mutex> lock(_mutex);
	auto real_filename = getRealFilename(filename);
	auto iter = _knownExtensions.find(real_filename);
	if(iter != _knownExtensions.end())
		_knownExtensions.erase(iter);
}

void ExtensionSystem::searchDirectory( const std::string& path ) {
	filesystem::forEachFileInDirectory(path, [this](const filesystem::path &p){
		if (p.extension().string() == DynamicLibrary::fileExtension())
			addDynamicLibrary(p.string());
	});
}

std::vector<ExtensionDescription> ExtensionSystem::extensions(const std::vector<std::pair<std::string, std::string> > &metaDataFilter) const
{
	std::unordered_map<std::string, std::unordered_set<std::string> > filterMap;

	for( auto &f : metaDataFilter )
	{
		filterMap[f.first].insert(f.second);
	}

	std::unique_lock<std::mutex> lock(_mutex);
	std::vector<ExtensionDescription> result;

	for(auto &i : _knownExtensions)
		for(auto &j : i.second.extensions) {
			// check all filters
			bool addExtension = true;

			for( auto &filter : filterMap ) {
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

	for(auto &i : _knownExtensions)
		for(auto &j : i.second.extensions)
			list.push_back(j);

	return list;
}

ExtensionDescription ExtensionSystem::_findDescription(const std::string& interface_name, const std::string& name, unsigned int version) const {
	for(auto &i : _knownExtensions)
		for(auto &j : i.second.extensions)
			if(j.interface_name() == interface_name && j.name() == name && j.version() == version)
				return j;

	return ExtensionDescription();
}

ExtensionDescription ExtensionSystem::_findDescription(const std::string& interface_name, const std::string& name) const {
	unsigned int highesVersion = 0;
	const ExtensionDescription *desc = nullptr;

	for( auto &i : _knownExtensions) {
		for( auto &j : i.second.extensions) {
			if(j.interface_name() == interface_name && j.name() == name && j.version() > highesVersion ) {
				highesVersion = j.version();
				desc = &j;
			}
		}
	}

	return desc ? *desc : ExtensionDescription();
}


void ExtensionSystem::setVerifyCompiler(bool enable) {
	_verify_compiler = enable;
}
