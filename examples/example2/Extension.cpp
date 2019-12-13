/// SPDX-FileCopyrightText: 2014-2020 Bernd Amend and Michael Adam
/// SPDX-License-Identifier: BSL-1.0
#include "Interface.hpp"
#include <extension_system/Extension.hpp>

class Extension : public Interface2 {
public:
    std::string test2() override {
        return "Hello from Interface2 Extension";
    }
};

// clang-format off
// export extension and add user defined metadata
EXTENSION_SYSTEM_EXTENSION(Interface2, Extension, "Example2Extension", 100, "Example 2 extension",
                           EXTENSION_SYSTEM_DESCRIPTION_ENTRY("author", "Alice Bobbens")
                           EXTENSION_SYSTEM_DESCRIPTION_ENTRY("vendor", "42 inc.")
                           EXTENSION_SYSTEM_DESCRIPTION_ENTRY("target_product", "example2"))
