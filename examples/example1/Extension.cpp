/// SPDX-FileCopyrightText: 2014-2020 Bernd Amend and Michael Adam
/// SPDX-License-Identifier: BSL-1.0
#include "Interface.hpp"
#include <extension_system/Extension.hpp>

class Extension : public Interface1 {
public:
    std::string test1() override {
        return "Hello from Extension1";
    }
};

EXTENSION_SYSTEM_EXTENSION(Interface1, Extension, "Example1Extension", 100, "Example 1 extension", EXTENSION_SYSTEM_NO_USER_DATA)
