/// SPDX-FileCopyrightText: 2014-2020 Bernd Amend and Michael Adam
/// SPDX-License-Identifier: BSL-1.0
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
