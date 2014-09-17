#include <extension_system/ExtensionSystem.hpp>
#include "Interface.hpp"
#include <iostream>

int main() {
	extension_system::ExtensionSystem extensionSystem;
	extensionSystem.searchDirectory(".");

	// iterate through all known Interface2 extensions and print metadata information
	for( auto extension : extensionSystem.extensions<Interface2>()) {
		std::cout<<"Extension: " << extension.name() << "(" << std::to_string(extension.version()) << ")" << std::endl;
		std::cout<<"Description: " << extension.description() << std::endl;
		std::cout<<"Author: "<< extension.getExtended()["author"] << std::endl;
		std::cout<<"Vendor: "<< extension.getExtended()["vendor"] << std::endl;
		std::cout<<"Target: "<< extension.getExtended()["target_product"] << std::endl;
	}

	// create extension
	std::shared_ptr<Interface2> e1 = extensionSystem.createExtension<Interface2>("Example2Extension");
	if(e1 != nullptr) {
		e1->test1();
	}
	std::cout<<"Done."<<std::endl;
	return 0;
}
