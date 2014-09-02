#include <extension_system/ExtensionSystem.hpp>
#include "Interface.hpp"
#include <iostream>

int main() {
	extension_system::ExtensionSystem extensionSystem;
	extensionSystem.searchDirectory("./");
	std::shared_ptr<Interface1> e1 = extensionSystem.createExtension<Interface1>("Extension1");
	if(e1 != nullptr)
		e1->test1();
	std::cout<<"Done."<<std::endl;
	return 0;
}
