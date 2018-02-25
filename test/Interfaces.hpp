/**
    @file
    @copyright
        Copyright Bernd Amend and Michael Adam 2014-2018
        Distributed under the Boost Software License, Version 1.0.
        (See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt)
*/
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
