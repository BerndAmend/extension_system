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

int main() {
    extension_system::ExtensionSystem extension_system;
    extension_system.searchDirectory(".");

    // iterate through all known Interface2 extensions and print metadata information
    for (const auto& extension : extension_system.extensions<Interface2>()) {
        std::cout << "Extension: " << extension.name() << "(" << extension.version() << ")\n"
                  << "Description: " << extension.description() << "\n"
                  << "Author: " << extension["author"] << "\n"
                  << "Vendor: " << extension["vendor"] << "\n"
                  << "Target: " << extension["target_product"] << "\n";
    }

    auto e = extension_system.createExtension<Interface2>("Example2Extension");

    if (e == nullptr) {
        std::cout << "couldn't load plugin\n";
        return -1;
    }

    std::cout << "output: " << e->test2() << "\n";
    return 0;
}
