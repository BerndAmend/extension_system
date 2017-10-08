/**
    @file
    @copyright
        Copyright Bernd Amend and Michael Adam 2014-2017
        Distributed under the Boost Software License, Version 1.0.
        (See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt)
*/
#include "Interface.hpp"
#include <extension_system/ExtensionSystem.hpp>
#include <iostream>

int main() {
    extension_system::ExtensionSystem extensionSystem;
    extensionSystem.searchDirectory(".");

    // iterate through all known Interface2 extensions and print metadata information
    for (const auto& extension : extensionSystem.extensions<Interface2>()) {
        std::cout << "Extension: " << extension.name() << "(" << extension.version() << ")\n"
                  << "Description: " << extension.description() << "\n"
                  << "Author: " << extension["author"] << "\n"
                  << "Vendor: " << extension["vendor"] << "\n"
                  << "Target: " << extension["target_product"] << "\n";
    }

    // create extension
    std::shared_ptr<Interface2> e1 = extensionSystem.createExtension<Interface2>("Example2Extension");
    if (e1 != nullptr) {
        e1->test1();
    }
    std::cout << "Done.\n";
    return 0;
}
