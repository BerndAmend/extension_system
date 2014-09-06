/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014
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

#include <extension_system/macros.hpp>
#include <extension_system/Extension.hpp>
#include <extension_system/DynamicLibrary.hpp>

namespace extension_system {

	struct ExtensionDescription {

		ExtensionDescription() {}
		ExtensionDescription(const std::unordered_map<std::string, std::string> &data) : _data(data) {}

		bool isValid() const {
			return !_data.empty();
		}

		std::string name() const {
			return get("name");
		}

		/**
		 * @brief version
		 * @return the version or 0 if the value couldn't be parsed or didn't exist
		 */
		unsigned int version() const {
			std::stringstream str;
			str << get("version");
			unsigned int result = 0;
			str >> result;
			return result;
		}

		std::string description() const {
			return get("description");
		}

		std::string interface_name() const {
			return get("interface_name");
		}

		std::string entry_point() const {
			return get("entry_point");
		}

		std::string library_filename() const {
			return get("library_filename");
		}

		std::unordered_map<std::string, std::string> getExtended() const {
			std::unordered_map<std::string, std::string> result = _data;

			result.erase("name");
			result.erase("version");
			result.erase("description");
			result.erase("interface_name");
			result.erase("entry_point");
			result.erase("library_filename");

			return result;
		}

		std::string get(const std::string &key) const {
			auto iter = _data.find(key);
			if(iter == _data.end()) {
				return "";
			} else {
				return iter->second;
			}
		}

		std::string operator[](const std::string &key) const {
			return get(key);
		}

		std::unordered_map<std::string, std::string> _data;
	};

	/**
	 * @brief The ExtensionSystem class
	 * thread-safe
	 */
	class ExtensionSystem {
	public:
		ExtensionSystem();
		ExtensionSystem(const ExtensionSystem&) = delete;
		~ExtensionSystem();
		ExtensionSystem& operator=(const ExtensionSystem&) = delete;

		/**
		 * @brief addDynamicLibrary
		 *	add a single dynamic library
		 * @param filename of the library
		 * @return true=the file contained at least one extension, false=file could not be opened or didn't contain an extension
		 */
		bool addDynamicLibrary(const std::string &filename);

		/**
		 * @brief removeDynamicLibrary
		 *	removes all extensions provided by the library from the list of known extensions
		 *	already loaded extensions are not affected by this call
		 * @param filename
		 */
		void removeDynamicLibrary(const std::string &filename);

		/**
		 * add a directory to extension-search-path
		 * after adding, the given path is checked for extensions
		 * @param path path to search in for extensions
		 */
		void searchDirectory(const std::string &path);

		/**
		 * get a list of all known extensions
		 */
		std::vector<ExtensionDescription> extensions();

		/**
		 * get a list of extensions, filtered by metadata
		 * Same metadata keys are treated as or-linked, different keys are and-linked treated.
		 * For example: {{"author", "Alice"}, {"author", "Bob"}, {"company", "MyCorp"}} will list all extensions whose authors are Alice or Bob and the company is "MyCorp"
		 * @param metaDataFilter Metadata to search extensions for. Use c++11 initializer lists for simple usage: {{"author", "Alice"}, {"company", "MyCorp"}}
		 * @return list of extensions
		 */
		std::vector<ExtensionDescription> extensions(const std::vector< std::pair< std::string, std::string > > &metaDataFilter);

		/**
		 * get a list of all known extensions of a specified interface type
		 */
		template<class T>
		std::vector<ExtensionDescription> extensions(std::vector< std::pair< std::string, std::string > > metaDataFilter={}) {
			metaDataFilter.push_back({"interface_name", extension_system::InterfaceName<T>::getString()});
			return extensions(metaDataFilter);
		}

		/**
		 * create an instance of an extension with a specified version
		 * loaded extensions can outlive the ExtensionSystem
		 * @param name name of extension to create
		 * @param version version of extension to create
		 * @return a instance of a extension class or a nullptr, if extension could not be instantiated
		 */
		template<class T>
		std::shared_ptr<T> createExtension(const std::string &name, unsigned int version) {
			std::unique_lock<std::mutex> lock(_mutex);
			return _createExtension<T>(extension_system::InterfaceName<T>::getString(), name, version);
		}

		/**
		 * create an instance of an extension. If the extension is available in multiple versions, the highest version will be instantiated
		 * loaded extensions can outlive the ExtensionSystem
		 * @param name name of extension to create
		 * @return a instance of a extension class or a nullptr, if extension could not be instantiated
		 */
		template<class T>
		std::shared_ptr<T> createExtension(const std::string &name) {
			std::unique_lock<std::mutex> lock(_mutex);
			auto desc = _findDescription(extension_system::InterfaceName<T>::getString(), name);
			if( desc.isValid() ) {
				return _createExtension<T>(extension_system::InterfaceName<T>::getString(), name, desc.version());
			}
			return std::shared_ptr<T>();
		}

		template<class T>
		ExtensionDescription findDescription(const std::shared_ptr<T> extension) const {
			std::unique_lock<std::mutex> lock(_mutex);
			return _findDescription(extension);
		}

		bool getDebugMessages() const { return _debug_messages; }
		void setDebugMessages(bool enable);

		bool getVerifyCompiler() const { return _verify_compiler; }

		// only has an effect before adding a directory
		void setVerifyCompiler(bool enable);
	private:

		ExtensionDescription _findDescription(const std::string &interface_name, const std::string& name, unsigned int version) const;
		ExtensionDescription _findDescription(const std::string& interface_name, const std::string& name) const;

		template<class T>
		std::shared_ptr<T> _createExtension(const std::string& interface_name, const std::string &name, unsigned int version ) {
			for(auto &i : _knownExtensions) {
				for(auto &j : i.second.extensions) {
					auto current_name = j.name();
					if( interface_name == j.interface_name() && current_name == name && j.version() == version ) {
						std::shared_ptr<DynamicLibrary> dynlib = i.second.dynamicLibrary.lock();
						if( dynlib == nullptr ) {
							try {
								dynlib = std::make_shared<DynamicLibrary>(i.first);
							} catch(std::exception &) {}
							i.second.dynamicLibrary = dynlib;
						}

						if(dynlib == nullptr)
							continue;

						auto func = dynlib->getProcAddress<T* (T *, const char **)>(j.entry_point());

						if( func != nullptr ) {
							T* ex = func(nullptr, nullptr);
							if( ex != nullptr) {
								_loadedExtensions[ex] = LoadedExtension(j, &i.second);
								// Frees an extension and unloads the containing library, if no references to that library are present.
								std::shared_ptr<bool> alive = _extension_system_alive;
								return std::shared_ptr<T>(ex, [this, alive, dynlib, func](T *obj){
									func(obj, nullptr);
									if(*alive) {
										std::unique_lock<std::mutex> lock(_mutex);
										_loadedExtensions.erase(obj);
									}
								});
							}
						}
					}
				}
			}
			return std::shared_ptr<T>();
		}

		template<class T>
		ExtensionDescription _findDescription(const std::shared_ptr<T> extension) const {
			auto i = _loadedExtensions.find(extension.get());
			if( i != _loadedExtensions.end() )
				return i->second._desc;
			else
				return ExtensionDescription();
		}

		struct LibraryInfo
		{
			LibraryInfo(const LibraryInfo&) = delete;
			LibraryInfo& operator=(const LibraryInfo&) = delete;
			LibraryInfo() {}
			LibraryInfo(const std::vector<ExtensionDescription>& ex) : extensions(ex) {}

			LibraryInfo &operator=(LibraryInfo &&other) {
				dynamicLibrary = std::move(other.dynamicLibrary);
				extensions = std::move(other.extensions);
				return *this;
			}

			std::weak_ptr<DynamicLibrary> dynamicLibrary;
			std::vector<ExtensionDescription> extensions;
		};

		struct LoadedExtension {
			LoadedExtension() : _info(nullptr){}
			LoadedExtension(const LoadedExtension&) =delete;
			LoadedExtension& operator=(const LoadedExtension&) =delete;

			LoadedExtension(const ExtensionDescription &desc, LibraryInfo *info)
				: _desc(desc), _info(info) {}

			LoadedExtension(LoadedExtension &&other) : _info(nullptr) {
				*this = std::move(other);
			}

			LoadedExtension &operator=(LoadedExtension &&other) {
				_desc = std::move(other._desc);
				_info = std::move(other._info);
				other._info = nullptr;
				return *this;
			}

			ExtensionDescription _desc;
			LibraryInfo *_info;
		};

		bool _verify_compiler;
		bool _debug_messages;
		// used to avoid removing extensions while destroying them from the loadedExtensions map
		std::shared_ptr<bool> _extension_system_alive;
		mutable std::mutex	_mutex;
		std::unordered_map<std::string, LibraryInfo> _knownExtensions;
		std::unordered_map<const void*, LoadedExtension> _loadedExtensions;
	};
}

/**
 * Stream operator to allow printing a Message in a human readable form.
 */
template <typename T, typename traits>
std::basic_ostream<T,traits> & operator << (std::basic_ostream<T,traits> &out, const extension_system::ExtensionDescription &obj) {
	out <<	"  name="<<obj.name()<<"\n"<<
			"  version="<<obj.version()<<"\n"
			"  description="<<obj.description()<<"\n"
			"  interface_name="<<obj.interface_name()<<"\n"
			"  entry_point="<<obj.entry_point()<<"\n"
			"  library_filename="<<obj.library_filename()<<"\n";

	auto extended = obj.getExtended();
	if(!extended.empty()) {
		out << "  Extended data:\n";
		for(auto &iter : extended)
			out << "    " << iter.first << " = " << iter.second << "\n";
	}

	return out ;
}
