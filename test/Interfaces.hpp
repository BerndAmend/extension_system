/// SPDX-FileCopyrightText: 2014-2020 Bernd Amend and Michael Adam
/// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <extension_system/Extension.hpp>
#include <string>

class IExt1 {
public:
    virtual int test1() = 0;
    virtual ~IExt1()    = default;
};
EXTENSION_SYSTEM_INTERFACE(IExt1)

namespace extension_system {
class IExt2 {
public:
    virtual std::string test2() = 0;
    virtual ~IExt2()            = default;
};
}

EXTENSION_SYSTEM_INTERFACE(extension_system::IExt2)
