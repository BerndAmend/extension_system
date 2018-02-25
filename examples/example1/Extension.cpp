/**
   @file
   @copyright
       Copyright Bernd Amend and Michael Adam 2014-2018
       Distributed under the Boost Software License, Version 1.0.
       (See accompanying file LICENSE_1_0.txt or copy at
       http://www.boost.org/LICENSE_1_0.txt)
*/
#include "Interface.hpp"
#include <extension_system/Extension.hpp>

class Extension : public Interface1 {
public:
    std::string test1() override {
        return "Hello from Extension1";
    }
};

EXTENSION_SYSTEM_EXTENSION(Interface1, Extension, "Example1Extension", 100, "Example 1 extension", EXTENSION_SYSTEM_NO_USER_DATA)
