/**
    @file
    @copyright
        Copyright Bernd Amend and Michael Adam 2014
        Distributed under the Boost Software License, Version 1.0.
        (See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt)
*/
#include "Interfaces.hpp"
#include <iostream>

class Ext1 : public IExt1
{
public:
    void test1() override
    {
        std::cout << "Hello from Ext1\n";
    }
};
// clang-format off
EXTENSION_SYSTEM_EXTENSION(IExt1, Ext1, "Ext1", 100, "extension 1 for testing purposes",
                           EXTENSION_SYSTEM_DESCRIPTION_ENTRY("Test1", "desc2")
                           EXTENSION_SYSTEM_DESCRIPTION_ENTRY("Test3", "desc3"))
// clang-format on

namespace test_namespace {
class Ext1_1 : public IExt1
{
public:
    void test1() override
    {
        std::cout << "Hello from Ext1_1\n";
    }
};
} // namespace test_namespace
// clang-format off
EXTENSION_SYSTEM_EXTENSION(IExt1, test_namespace::Ext1_1, "Ext1", 110, "extension 2 for testing purposes",
                           EXTENSION_SYSTEM_DESCRIPTION_ENTRY("Test1", "desc1"))
// clang-format on

class Ext2 : public extension_system::IExt2
{
public:
    void test2() override
    {
        std::cout << "Hello from Ext2\n";
    }
};
// clang-format off
EXTENSION_SYSTEM_EXTENSION(extension_system::IExt2, Ext2, "Ext2", 100, "extension 3 for testing purposes",
                           EXTENSION_SYSTEM_DESCRIPTION_ENTRY("Test1", "desc1")
                           EXTENSION_SYSTEM_DESCRIPTION_ENTRY("Test2", "desc2")
                           EXTENSION_SYSTEM_DESCRIPTION_ENTRY("Test3", "desc3"))
// clang-format on
