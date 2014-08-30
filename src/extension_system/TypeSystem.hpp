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
#include <cstdint>
#include <extension_system/macros.hpp>
#include <type_traits>

#include <typeinfo>

#if defined(EXTENSION_SYSTEM_COMPILER_GNU)
	#include <cxxabi.h>
#endif

namespace extension_system {
	typedef uint32_t type_id;

	template <type_id a, type_id b, type_id c, type_id d>
	struct EXTENSION_SYSTEM_TYPE_ID {
		static const type_id value = (((((d << 24) | c) << 16) | b) << 8) | a;
	};
}

/**
 * You have to call this macro with the fully qualified typename
 * Using the getID function is unsafe and may result in wrong results.
 */
#define EXTENSION_SYSTEM_DECLARE_TYPE(T, a, b, c, d) \
	namespace extension_system { \
		template<> class TypeSystem<T> : public std::true_type  { \
		public: \
			inline static std::string getString() { return #T ; } \
			EXTENSION_SYSTEM_CONSTEXPR inline static const std::type_info &get() { return typeid(T); } \
			static const type_id id = EXTENSION_SYSTEM_TYPE_ID<a, b, c, d>::value; \
		}; \
	}

namespace extension_system {

template<typename T, typename Enable = void>
class TypeSystem : public std::false_type {
public:
	/**
	 * This is the only real rtti dependency,
	 * rtti is not required if template definitions exist for all used types
	 */
	static std::string getString() EXTENSION_SYSTEM_NOEXCEPT {
			const std::type_info &ti = typeid(T);
			std::string name = ti.name();
	#if defined(EXTENSION_SYSTEM_COMPILER_MSVC)
			const std::string struct_str = "struct ";
			const std::string class_str = "class ";
			if(std::equal(struct_str.begin(), struct_str.end(), name.begin())) {
				name = name.substr(struct_str.length());
			} else if(std::equal(class_str.begin(), class_str.end(), name.begin())) {
				name = name.substr(class_str.length());
			}
	#elif defined(EXTENSION_SYSTEM_COMPILER_GNU)
			char *realname = abi::__cxa_demangle(ti.name(), 0, 0, nullptr);
			name = std::string(realname);
			free(realname);
	#else
			#error Unknown compiler
	#endif
			return name;
	}

	static const std::type_info &get() {
		return typeid(T);
	}
};

}

// definitions for all base types
EXTENSION_SYSTEM_DECLARE_TYPE(bool, 'b','o','o','l')

EXTENSION_SYSTEM_DECLARE_TYPE(int8_t, ' ','s','i','8')
EXTENSION_SYSTEM_DECLARE_TYPE(uint8_t, ' ','u','i','8')

EXTENSION_SYSTEM_DECLARE_TYPE(int16_t, 's','i','1','6')
EXTENSION_SYSTEM_DECLARE_TYPE(uint16_t, 'u','i','1','6')

EXTENSION_SYSTEM_DECLARE_TYPE(int32_t, 's','i','3','2')
EXTENSION_SYSTEM_DECLARE_TYPE(uint32_t, 'u','i','3','2')

EXTENSION_SYSTEM_DECLARE_TYPE(int64_t, 's','i','6','4')
EXTENSION_SYSTEM_DECLARE_TYPE(uint64_t, 'u','i','6','4')

EXTENSION_SYSTEM_DECLARE_TYPE(float, 'f','l','o','a')
EXTENSION_SYSTEM_DECLARE_TYPE(double, 'd','o','u','b')

EXTENSION_SYSTEM_DECLARE_TYPE(std::string, ' ','s','t','r')
EXTENSION_SYSTEM_DECLARE_TYPE(std::wstring, 'w','s','t','r')
