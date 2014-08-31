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

		std::unordered_map<std::string, std::string> getExtended() const {
			std::unordered_map<std::string, std::string> result = _data;

			result.erase("name");
			result.erase("version");
			result.erase("description");
			result.erase("interface_name");
			result.erase("entry_point");

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
		ExtensionSystem(const ExtensionSystem&) =delete;
		ExtensionSystem& operator=(const ExtensionSystem&) =delete;

		/**
		 * @brief addDynamicLibrary
		 *	add a single dynamic library
		 * @param filename of the library
		 * @return true=the file contained at least one extension, false=file could not be opened or didn't contain an extension
		 */
		bool addDynamicLibrary(const std::string &filename);

		/**
		 * add a directory to extension-search-path
		 * after adding, the given path is checked for extensions
		 * @param path path to search in for extensions
		 */
		void searchDirectory(const std::string& path);

		/**
		 * get a list of all known extensions
		 */
		std::vector<ExtensionDescription> extensions();

		/**
		 * get a list of all known extensions of a specified interface type
		 */
		template<class T>
		std::vector<ExtensionDescription> extensions() {
			return _extensions(extension_system::InterfaceName<T>::getString());
		}

		/**
		 * create an instance of an extension with a specified version
		 * @param name name of extension to create
		 * @param version version of extension to create
		 * @return a instance of a extension class or a nullptr, if extension could not be instantiated
		 */
		template<class T>
		std::shared_ptr<T> createExtension(const std::string &name, unsigned int version) {
			std::unique_lock<std::mutex> lock(_mutex);
			auto desc = _findDescription(name, version);
			if( desc != nullptr && desc->interface_name() == extension_system::InterfaceName<T>::getString() ) {
				return _createExtension<T>(name, version);
			}
			return std::shared_ptr<T>();
		}

		/**
		 * create an instance of an extension. If the extension is available in multiple versions, the highest version will be instantiated
		 * @param name name of extension to create
		 * @return a instance of a extension class or a nullptr, if extension could not be instantiated
		 */
		template<class T>
		std::shared_ptr<T> createExtension(const std::string &name) {
			std::unique_lock<std::mutex> lock(_mutex);
			auto desc = _findDescription(name);
			if( desc != nullptr && desc->interface_name() == extension_system::InterfaceName<T>::getString() ) {
				return _createExtension<T>(name, desc->version());
			}
			return std::shared_ptr<T>();
		}

		template<class T>
		const ExtensionDescription *findDescription(const std::shared_ptr<T> extension) const {
			std::unique_lock<std::mutex> lock(_mutex);
			return _findDescription(extension);
		}

		const ExtensionDescription *findDescription(const std::string& name, unsigned int version) const {
			std::unique_lock<std::mutex> lock(_mutex);
			return _findDescription(name, version);
		}
		const ExtensionDescription *findDescription(const std::string& name) const {
			std::unique_lock<std::mutex> lock(_mutex);
			return _findDescription(name);
		}

		bool getDebugMessages() const { return _debug_messages; }
		void setDebugMessages(bool enable);

		bool getVerifyCompiler() const { return _verify_compiler; }

		// only has an effect before adding a directory
		void setVerifyCompiler(bool enable);
	private:

		const ExtensionDescription *_findDescription(const std::string& name, unsigned int version) const;
		const ExtensionDescription *_findDescription(const std::string& name) const;

		/**
		 * frees an extension and unloads the containing library, if no references to that library are present
		 * @param extension extension to be freed
		 */
		template<class T>
		void freeExtension( T *extension ) {
			std::unique_lock<std::mutex> lock(_mutex);
			if( extension == nullptr ) //HINT: better do an assert here?
				return;

			auto i = _loadedExtensions.find(extension);

			if( i != _loadedExtensions.end() ) {
				DynamicLibrary *dynlib = i->second._info->dynamicLibrary.get();
				auto func =
					dynlib->getProcAddress<T*(T *, const char **)>(i->second._desc.entry_point());

				if( func != nullptr ) {
					func(extension, nullptr);
					i->second._info->references--;
					if(i->second._info->references == 0)
						i->second._info->dynamicLibrary.reset();
				}
				_loadedExtensions.erase(i);
			}
		}

		template<class T>
		std::shared_ptr<T> _createExtension( const std::string &name, unsigned int version ) {
			for( auto i = _knownExtensions.begin(); i != _knownExtensions.end(); i++ ) {
				for( auto j = i->second.extensions.begin(); j != i->second.extensions.end(); j++) {
					auto current_name = j->name();
					if( current_name == name && j->version() == version ) {
						if( i->second.references == 0 ) {
							try {
								i->second.dynamicLibrary.reset(new DynamicLibrary(i->first));
							} catch(std::exception &) {}
						}

						if(i->second.dynamicLibrary == nullptr)
							continue;

						auto func = i->second.dynamicLibrary->getProcAddress<T* (T *, const char **)>(j->entry_point());

						if( func != nullptr ) {
							T* ex = func(nullptr, nullptr);
							if( ex == nullptr && i->second.references == 0) {
								i->second.dynamicLibrary.reset();
							} else {
								i->second.references++;
								_loadedExtensions[ex] = LoadedExtension(*j, &i->second);
								// FIXME: the following line is broken
								// it will crash if the object is still alive when the ExtensionSystem is destroyed
								return std::shared_ptr<T>(ex, [&](T *obj){freeExtension(obj);});
							}
						}
					}
				}
			}
			return std::shared_ptr<T>();
		}

		template<class T>
		const ExtensionDescription *_findDescription(const std::shared_ptr<T> extension) const {
			auto i = _loadedExtensions.find(extension.get());
			if( i != _loadedExtensions.end() )
				return &(i->second._desc);
			else
				return nullptr;
		}

		std::vector<ExtensionDescription> _extensions(const std::string& interfaceName);

		struct LibraryInfo
		{
			LibraryInfo(const LibraryInfo&) = delete;
			LibraryInfo& operator=(const LibraryInfo&) = delete;
			LibraryInfo() : references(0) {}
			LibraryInfo(const std::vector<ExtensionDescription>& ex) : extensions(ex), references(0) {}
			LibraryInfo(LibraryInfo &&other) : references(0) {
				*this = std::move(other);
			}

			LibraryInfo &operator=(LibraryInfo &&other) {
				dynamicLibrary = std::move(other.dynamicLibrary);
				extensions = std::move(other.extensions);
				references = std::move(other.references);

				other.references = 0;

				return *this;
			}

			std::unique_ptr<DynamicLibrary> dynamicLibrary;
			std::vector<ExtensionDescription> extensions;
			int references;
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
			"  entry_point="<<obj.entry_point()<<"\n";

	auto extended = obj.getExtended();
	if(!extended.empty()) {
		out << "  Extended data:\n";
		for(auto iter = extended.begin(); iter != extended.end(); ++iter)
			out << "    " << iter->first << " = " << iter->second << "\n";
	}

	return out ;
}
