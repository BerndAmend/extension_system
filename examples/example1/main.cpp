/**
   @file
   @copyright
       Copyright Bernd Amend and Michael Adam 2014-2018
       Distributed under the Boost Software License, Version 1.0.
       (See accompanying file LICENSE_1_0.txt or copy at
       http://www.boost.org/LICENSE_1_0.txt)
*/
#include "Interface.hpp"
#include <extension_system/ExtensionSystem.hpp>
#include <iostream>

int main() try {
    extension_system::ExtensionSystem extension_system;
    extension_system.searchDirectory(".");
    auto e = extension_system.createExtension<Interface1>("Example1Extension");
    if (e == nullptr) {
        std::cout << "couldn't load plugin\n";
        return -1;
    }
    std::cout << "output: " << e->test1() << "\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << "Caught exception " << e.what() << "\n";
    return -1;
}
