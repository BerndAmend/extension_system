#include <extension_system/ExtensionSystem.hpp>
#include "Interface.hpp"
#include <iostream>

int main() {
	extension_system::ExtensionSystem extensionSystem;
	extensionSystem.searchDirectory(".");

	std::vector<extension_system::ExtensionDescription> interface2Extensions = extensionSystem.extensions<Interface2>();
	std::vector<extension_system::ExtensionDescription>::iterator iter;

	// iterate through all known Interface2 extensions and print metadata information
	for( iter = interface2Extensions.begin(); iter != interface2Extensions.end(); iter++ ) {
		std::cout<<"Extension: " << iter->name() << "(" << std::to_string(iter->version()) << ")" << std::endl;
		std::cout<<"Description: " << iter->description() << std::endl;
		std::cout<<"Author: "<< iter->getExtended()["author"] << std::endl;
		std::cout<<"Vendor: "<< iter->getExtended()["vendor"] << std::endl;
		std::cout<<"Target: "<< iter->getExtended()["target_product"] << std::endl;
	}

	// create extension
	std::shared_ptr<Interface2> e1 = extensionSystem.createExtension<Interface2>("Example2Extension");
	if(e1 != nullptr) {
		e1->test1();
	}
	std::cout<<"Done."<<std::endl;
	return 0;
}
