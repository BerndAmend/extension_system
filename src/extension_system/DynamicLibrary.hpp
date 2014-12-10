/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014
		Distributed under the Boost Software License, Version 1.0.
		(See accompanying file LICENSE_1_0.txt or copy at
		http://www.boost.org/LICENSE_1_0.txt)
*/
#pragma once

#include <string>
#include <functional>

namespace extension_system {

	class DynamicLibrary {
	public:
		DynamicLibrary(const std::string &filename);
		DynamicLibrary(const DynamicLibrary&) =delete;
		DynamicLibrary& operator=(const DynamicLibrary&) =delete;
		~DynamicLibrary();

		std::string getFilename() const;

		const void *getHandle() const;

		void *getProcAddress(const std::string &name) const;

		template<typename FunctionSignature>
		std::function<FunctionSignature> getProcAddress(const std::string &name) const {
			return reinterpret_cast<FunctionSignature*>(getProcAddress(name));
		}

		static std::string fileExtension();

	private:
		const std::string _filename;
		void * const _handle;

		static const std::string _file_extension;
	};

}
