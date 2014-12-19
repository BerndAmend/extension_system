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
#include <functional>

#include <extension_system/macros.hpp>
#include <extension_system/Extension.hpp>
#include <extension_system/DynamicLibrary.hpp>

namespace extension_system {

	/**
	 * Structure that describes an extenstion.
	 * Basically an extension is described by
	 * \li the interface it implements,
	 * \li its name and
	 * \li its version.
	 * Additionally a extension creator can add metadata to further describe the extension.
	 */
	struct ExtensionDescription {

		ExtensionDescription() {}
		ExtensionDescription(const std::unordered_map<std::string, std::string> &data) : _data(data) {}

		/**
		 * Returns if the extension is valid. An extension is invalid if the describing data structure was not found within the shared module (.so/.dll ...)
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
		unsigned int version() const {
			std::stringstream str(get("version"));
			unsigned int result = 0;
			str >> result;
			return result;
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

		/**
		 * Returns meta data identified by key
		 * @param key Key to retrieve metadata for
		 * @return Metadata associated with key or an empty string if key was not found in meta data
		 */
		std::string get(const std::string &key) const {
			auto iter = _data.find(key);
			if(iter == _data.end()) {
				return std::string();
			} else {
				return iter->second;
			}
		}

		/**
		 * Equivalent to get()
		 */
		std::string operator[](const std::string &key) const {
			return get(key);
		}

		bool operator==(const ExtensionDescription &desc) const {
			return _data == desc._data;
		}

		std::string toString() const {
			std::stringstream out;
			out <<	"  name="<<name()<<"\n"<<
					"  version="<<version()<<"\n"
					"  description="<<description()<<"\n"
					"  interface_name="<<interface_name()<<"\n"
					"  library_filename="<<library_filename()<<"\n";

			const auto extended = getExtended();
			if(!extended.empty()) {
				out << "  Extended data:\n";
				for(const auto &iter : _data)
					out << "    " << iter.first << " = " << iter.second << "\n";
			}
			return out.str();
		}

	private:

		std::string entry_point() const {
			return get("entry_point");
		}

		std::unordered_map<std::string, std::string> _data;

		friend class ExtensionSystem;
	};

	/**
	 * @brief The ExtensionSystem class
	 * thread-safe
	 */
	class ExtensionSystem {
	public:
		ExtensionSystem();
		ExtensionSystem(const ExtensionSystem&) = delete;
		ExtensionSystem& operator=(const ExtensionSystem&) = delete;

		/**
		 * Scans a dynamic library file for extensions and adds these extensions to the list of known extensions.
		 * @param filename File name of the library
		 * @return true=the file contained at least one extension, false=file could not be opened or didn't contain an extension
		 */
		bool addDynamicLibrary(const std::string &filename);

		/**
		 * Removes all extensions provided by the library from the list of known extensions
		 * Currently instantiated extensions are not affected by this call
		 * @param filename File name of library to remove
		 */
		void removeDynamicLibrary(const std::string &filename);

		/**
		 * Scans a directory for dynamic libraries and calls addDynamicLibrary for every found library
		 * @param path Path to search in for extensions
		 * @param recursive Search in subdirectories
		 */
		void searchDirectory(const std::string &path, bool recursive=false);

		/**
		 * Scans a directory for dynamic libraries and calls addDynamicLibrary for every found library whose file begins with required_prefix
		 * @param path Path to search in for extensions
		 * @param required_prefix Required prefix for libraries
		 * @param recursive Search in subdirectories
		 */
		void searchDirectory(const std::string &path, const std::string &required_prefix, bool recursive=false);

		/**
		 * Returns a list of all known extensions
		 */
		std::vector<ExtensionDescription> extensions() const;

		/**
		 * Returns a list of extensions, filtered by metadata.
		 * Same metadata keys are treated as or-linked, different keys are and-linked treated.
		 * For example: {{"author", "Alice"}, {"author", "Bob"}, {"company", "MyCorp"}} will list all extensions whose authors are Alice or Bob and the company is "MyCorp"
		 * @param metaDataFilter Metadata to search extensions for. Use c++11 initializer lists for simple usage: {{"author", "Alice"}, {"company", "MyCorp"}}
		 * @return List of extensions
		 */
		std::vector<ExtensionDescription> extensions(const std::vector< std::pair< std::string, std::string > > &metaDataFilter) const;

		/**
		 * Returns a list of all known extensions of a specified interface type
		 */
		template<class T>
		std::vector<ExtensionDescription> extensions(std::vector< std::pair< std::string, std::string > > metaDataFilter={}) const {
			metaDataFilter.push_back({"interface_name", extension_system::InterfaceName<T>::getString()});
			return extensions(metaDataFilter);
		}

		/**
		 * Creates an instance of an extension with a specified version
		 * Instantiated extension can outlive the ExtensionSystem (instance)
		 * @param name Name of extension to create
		 * @param version Version of extension to create
		 * @return An instance of an extension class or nullptr, if extension could not be instantiated
		 */
		template<class T>
		std::shared_ptr<T> createExtension(const std::string &name, unsigned int version) {
			std::unique_lock<std::mutex> lock(_mutex);
			auto desc = _findDescription(extension_system::InterfaceName<T>::getString(), name, version);
			if( desc.isValid() ) {
				return _createExtension<T>(desc);
			}
			return std::shared_ptr<T>();
		}

		/**
		 * Creates an instance of an extension. If the extension is available in multiple versions, the highest version will be instantiated
		 * Instantiated extension can outlive the ExtensionSystem (instance)
		 * @param name Name of extension to create
		 * @return An instance of an extension class or nullptr, if extension could not be instantiated
		 */
		template<class T>
		std::shared_ptr<T> createExtension(const std::string &name) {
			std::unique_lock<std::mutex> lock(_mutex);
			const auto desc = _findDescription(extension_system::InterfaceName<T>::getString(), name);
			if( desc.isValid() ) {
				return _createExtension<T>(desc);
			}
			return std::shared_ptr<T>();
		}

		/**
		 * Creates an instance of an extension using an ExtensionDescription.
		 * Instantiated extension can outlive the ExtensionSystem (instance)
		 * @param desc desc as returned by extension(...)
		 * @return an instance of an extension class or a nullptr, if extension could not be instantiated
		 */
		template<class T>
		std::shared_ptr<T> createExtension(const ExtensionDescription &desc) {
			std::unique_lock<std::mutex> lock(_mutex);
			return _createExtension<T>(desc);
		}

		/**
		 * Create an instance of an extension using a metaDataFilter.
		 * If multiple versions match the filter the first one as returned by extensions(metaDataFilter) is choosen.
		 * Instantiated extension can outlive the ExtensionSystem (instance)
		 * @param metaDataFilter metaDataFilter that should be used to create the extension
		 * @return an instance of an extension class or a nullptr, if extension could not be instantiated
		 */
		template<class T>
		std::shared_ptr<T> createExtension(const std::vector< std::pair< std::string, std::string > > &metaDataFilter) {
			const auto ext = extensions<T>(metaDataFilter);
			if(ext.empty())
				return std::shared_ptr<T>();
			return _createExtension<T>(ext[0]);
		}

		/**
		 * @param extension Instance of an extension.
		 * @returns ExtensionDescription of extension.
		 * If extension was not created by this extension system instance, an empty ExtensionDescription will be returned.
		 */
		template<class T>
		ExtensionDescription findDescription(const std::shared_ptr<T> &extension) const {
			std::unique_lock<std::mutex> lock(_mutex);
			return _findDescription(extension);
		}

		/**
		 * Sets a message handler.
		 * A message handler is a function that should be called if the ExtensionSystem detects an non fatal error while adding a library.
		 * The default message handler prints to std::cerr
		 * @param func Message handler function or nullptr if messages should be disabled.
		 */
		void setMessageHandler(std::function<void(const std::string &)> &func) {
			if(func == nullptr)
				_message_handler = [](const std::string &){};
			else
				_message_handler = func;
		}

		void setMessageHandler(std::nullptr_t) {
			_message_handler = [](const std::string &){};
		}

		bool getVerifyCompiler() const { return _verify_compiler; }

		/**
		 * enables or disables the compiler match verification
		 * only effects libraries that are added afterwards
		 */
		void setVerifyCompiler(bool enable);

		/**
		 * @brief setCheckForUPXCompression
		 * The check is costly and not required most of the time
		 * @param enable
		 */
		void setCheckForUPXCompression(bool enable) {
			_check_for_upx_compression = enable;
		}

		bool getCheckForUPXCompression() const {
			return _check_for_upx_compression;
		}

	private:

		bool _addDynamicLibrary(const std::string &filename, std::vector<char> &buffer);

		ExtensionDescription _findDescription(const std::string &interface_name, const std::string& name, unsigned int version) const;
		ExtensionDescription _findDescription(const std::string& interface_name, const std::string& name) const;

		template<class T>
		std::shared_ptr<T> _createExtension(const ExtensionDescription &desc) {
			if(!desc.isValid())
				return std::shared_ptr<T>();

			for(auto &i : _known_extensions) {
				for(auto &j : i.second.extensions) {
					if(j == desc) {
						std::shared_ptr<DynamicLibrary> dynlib = i.second.dynamic_library.lock();
						if( dynlib == nullptr ) {
							dynlib = std::make_shared<DynamicLibrary>(i.first);
							if(!dynlib->isValid()) {
								_message_handler("_createExtension: " + dynlib->getLastError());
							}
							i.second.dynamic_library = dynlib;
						}

						if(dynlib == nullptr)
							continue;

						const auto func = dynlib->getProcAddress<T* (T *, const char **)>(j.entry_point());

						if( func != nullptr ) {
							T* ex = func(nullptr, nullptr);
							if( ex != nullptr) {
								_loaded_extensions[ex] = j;
								// Frees an extension and unloads the containing library, if no references to that library are present.
								std::weak_ptr<bool> alive = _extension_system_alive;
								return std::shared_ptr<T>(ex, [this, alive, dynlib, func](T *obj){
									func(obj, nullptr);
									if(!alive.expired()) {
										std::unique_lock<std::mutex> lock(_mutex);
										_loaded_extensions.erase(obj);
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
			const auto i = _loaded_extensions.find(extension.get());
			if( i != _loaded_extensions.end() )
				return i->second;
			else
				return ExtensionDescription();
		}

		struct LibraryInfo {
			LibraryInfo() {}
			LibraryInfo(const LibraryInfo&) = delete;
			LibraryInfo& operator=(const LibraryInfo&) = delete;
			LibraryInfo& operator=(LibraryInfo &&) = default;
			LibraryInfo(const std::vector<ExtensionDescription>& ex) : extensions(ex) {}

			std::weak_ptr<DynamicLibrary> dynamic_library;
			std::vector<ExtensionDescription> extensions;
		};

		bool _verify_compiler = true;
		bool _check_for_upx_compression = false;

		std::function<void(const std::string &)> _message_handler;
		// used to avoid removing extensions while destroying them from the loadedExtensions map
		std::shared_ptr<bool> _extension_system_alive;
		mutable std::mutex _mutex;
		std::unordered_map<std::string, LibraryInfo> _known_extensions;
		std::unordered_map<const void*, ExtensionDescription> _loaded_extensions;

		// The following strings are used to find the exported classes in the dll/so files
		// The strings are concatenated at runtime to avoid that they are found in the ExtensionSystem binary.
		const std::string desc_base  = "EXTENSION_SYSTEM_METADATA_DESCRIPTION_";
		const std::string desc_start = ExtensionSystem::desc_base + "START";
		const std::string desc_end   = ExtensionSystem::desc_base + "END";
		const std::string upx_string = "UPX";
		const std::string upx_exclamation_mark_string = ExtensionSystem::upx_string + "!";
	};
}
