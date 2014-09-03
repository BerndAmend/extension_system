/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014
		Distributed under the Boost Software License, Version 1.0.
		(See accompanying file LICENSE_1_0.txt or copy at
		http://www.boost.org/LICENSE_1_0.txt)
*/
#include <extension_system/ExtensionSystem.hpp>

#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <extension_system/string.hpp>
#include <extension_system/filesystem.hpp>

using namespace extension_system;

const std::string desc_base = "EXTENSION_SYSTEM_METADATA_DESCRIPTION_";
const std::string desc_start = desc_base + "START";
const std::string desc_end = desc_base + "END";
const std::string upx_string = "UPX";
const std::string upx_exclamation_mark_string = upx_string + "!";

ExtensionSystem::ExtensionSystem()
	: _verify_compiler(true), _debug_messages(false) {}

bool ExtensionSystem::addDynamicLibrary(const std::string &filename) {
	std::unique_lock<std::mutex> lock(_mutex);
	filesystem::path filen(filename);

	if (!filesystem::exists(filen)) { // check if the file exists
		if(filesystem::exists(filesystem::path(filename + DynamicLibrary::fileExtension()))) {
			filen = filename + DynamicLibrary::fileExtension();
		} else {
			if(_debug_messages)
				std::cerr<<"ExtensionSystem addDynamicLibrary file doesn't exist filename="<<(filename + DynamicLibrary::fileExtension())<<std::endl;
			return false;
		}
		if(_debug_messages)
			std::cerr<<"ExtensionSystem addDynamicLibrary file doesn't exist filename="<<filename<<std::endl;
	}

	if(filesystem::is_directory(filen)) {
		if(_debug_messages)
			std::cerr<<"ExtensionSystem addDynamicLibrary doesn't support adding directories directory="<<filename<<std::endl;
		return false;
	}

	std::string filePath = canonical(filen).generic_string();

	auto already_loaded = _knownExtensions.find(filePath);

	// don't reload library, if there are already references to one contained extension
	if( already_loaded!=_knownExtensions.end() && already_loaded->second.references != 0 ) {
		return false;
	}

	std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);

	std::ifstream::pos_type file_length = file.tellg();

	if(file_length <= 0) {
		if(_debug_messages)
			std::cerr<<"invalid file size"<<std::endl;
		return false;
	}

	file.seekg(0, std::ios::beg);

	std::vector<char> str(static_cast<unsigned int>(file_length));
	file.read(str.data(), file_length);
	file.close();

	std::string t(str.data(), str.size());

	std::size_t found = t.find(desc_start);

	std::vector<std::unordered_map<std::string, std::string> > data;

	while(found != std::string::npos) {
		std::size_t start = found;

		// search the end tag
		found=t.find(desc_end, found+1);
		std::size_t end = found;

		if(end == std::string::npos) { // end tag not found
			if(_debug_messages)
				std::cerr<<"ExtensionSystem filename="<<filename<<" end tag was missing"<<std::endl;
			break;
		}

		// search the next start tag and check if it is interleaved with current section
		found=t.find(desc_start, start+1);

		if(found < end ) {
			if(_debug_messages)
				std::cerr<<"ExtensionSystem filename="<<filename<<" found a start tag before the expected end tag"<<std::endl;
			continue;
		}

		const std::string raw = std::string(str.data()+start, end-start-1);

		std::vector<std::string> strs;
		strs = split(raw, '\0');

		bool failed = false;
		std::unordered_map<std::string, std::string> result;
		for(auto &iter : strs) {
			std::size_t pos = iter.find('=');
			if(pos == std::string::npos) {
				if(_debug_messages)
					std::cerr<<"ExtensionSystem filename="<<filename<<" '=' is missing ("<<iter<<"). Ignore entry."<<std::endl;
				failed = true;
				break;
			}
			std::string key = iter->substr(0, pos);
			std::string value = iter->substr(pos+1);
			if(result.find(key) != result.end()) {
				if(_debug_messages)
					std::cerr<<"ExtensionSystem filename="<<filename<<" duplicate key ("<<key<<") found. Ignore entry"<<std::endl;
				failed = true;
				break;
			}
			result[key] = value;
		}

		result["library_filename"] = filePath;

		if(failed) {
			// we already printed an error
		} else if(!result.empty()) {
			data.push_back(result);
		} else {
			if(_debug_messages)
				std::cerr<<"ExtensionSystem filename="<<filename<<" metadata description didn't contain any data, ignore it"<<std::endl;
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
				if(_debug_messages)
					std::cerr<<"ExtensionSystem filename="<<filename<<" name was empty or not set"<<std::endl;
				continue;
			}

			if(desc.description().empty()) {
				if(_debug_messages)
					std::cerr<<"ExtensionSystem filename="<<filename<<" name="<<desc.name()<<" description was empty or not set"<<std::endl;
				continue;
			}

			if(desc.interface_name().empty()) {
				if(_debug_messages)
					std::cerr<<"ExtensionSystem filename="<<filename<<" name="<<desc.name()<<" interface_name was empty or not set"<<std::endl;
				continue;
			}

			if(desc.entry_point().empty()) {
				if(_debug_messages)
					std::cerr<<"ExtensionSystem filename="<<filename<<" name="<<desc.name()<<" entry_point was empty or not set"<<std::endl;
				continue;
			}

			if(desc.version() == 0) {
				if(_debug_messages)
					std::cerr<<"ExtensionSystem filename="<<filename<<" name="<<desc.name()<<": version number was invalid or 0"<<std::endl;
				continue;
			}

			extension_list.push_back(desc);
		} else {
			if(_debug_messages)
				std::cerr<<"ExtensionSystem: Ignore file "<<
							filename<<". Compilation options didn't match or were invalid "<<
							"(version="<<desc[desc_start]<<
							" compiler="<<desc["compiler"]<<
							" compiler_version="<<desc["compiler_version"]<<
							" build_type="<<desc["build_type"]<<
							" expected "<<
							" version="<<EXTENSION_SYSTEM_STR(EXTENSION_SYSTEM_EXTENSION_API_VERSION)<<
							" compiler="<<EXTENSION_SYSTEM_COMPILER<<
							" compiler_version="<<EXTENSION_SYSTEM_COMPILER_VERSION_STR<<
							" build_type="<<EXTENSION_SYSTEM_BUILD_TYPE<<
							")"<<std::endl;
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
			if(_debug_messages)
				std::cerr<<"ExtensionSystem: Couldn't find any extensions in file "<<filename<<", it seems the file is compressed using upx. "<<std::endl;
		}

		return false;
	}
}

void ExtensionSystem::searchDirectory( const std::string& path ) {
	for(auto &p : filesystem::getDirectoryContent(path))
		if (p.extension().string() == DynamicLibrary::fileExtension())
			addDynamicLibrary(p.string());
}

std::vector<ExtensionDescription> ExtensionSystem::extensions() {
	std::unique_lock<std::mutex> lock(_mutex);
	std::vector<ExtensionDescription> list;

	for(auto &i : _knownExtensions)
		for(auto &j : i.second.extensions)
			list.push_back(j);

	return list;
}

std::vector<ExtensionDescription> ExtensionSystem::_extensions(const std::string& interface_name) {
	std::unique_lock<std::mutex> lock(_mutex);
	std::vector<ExtensionDescription> result;

	for(auto &i : _knownExtensions)
		for(auto &j : i.second.extensions)
			if(j.interface_name() == interface_name)
				result.push_back(j);

	return result;
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

void ExtensionSystem::setDebugMessages(bool enable) {
	_debug_messages  = enable;
}

void ExtensionSystem::setVerifyCompiler(bool enable) {
	_verify_compiler = enable;
}
