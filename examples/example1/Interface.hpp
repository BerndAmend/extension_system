/// SPDX-FileCopyrightText: 2014-2020 Bernd Amend and Michael Adam
/// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <extension_system/Extension.hpp>
#include <string>

class Interface1 {
public:
    virtual std::string test1() = 0;
    virtual ~Interface1()       = default;
};
EXTENSION_SYSTEM_INTERFACE(Interface1)
