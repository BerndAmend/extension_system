/// SPDX-FileCopyrightText: 2014-2020 Bernd Amend and Michael Adam
/// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <extension_system/Extension.hpp>
#include <string>

class Interface2 {
public:
    virtual std::string test2() = 0;
    virtual ~Interface2()       = default;
};
EXTENSION_SYSTEM_INTERFACE(Interface2)
