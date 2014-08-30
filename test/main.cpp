/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014
		Distributed under the Boost Software License, Version 1.0.
		(See accompanying file LICENSE_1_0.txt or copy at
		http://www.boost.org/LICENSE_1_0.txt)
*/
#include <extension_system/ExtensionSystem.hpp>
#include <iostream>

#include "Interfaces.hpp"

using namespace extension_system;

int main() {
	ExtensionSystem extensionSystem;
	extensionSystem.setDebugMessages(true);

	extensionSystem.searchDirectory("./");
	extensionSystem.searchDirectory("../lib");
	auto extensions =  extensionSystem.extensions();

	for( auto i = extensions.begin(); i != extensions.end(); i++ )
		std::cout << *i << "\n";
	std::cout << "\nExtensions with interface IExt1:\n";
	extensions = extensionSystem.extensions<IExt1>();
	for( auto i = extensions.begin(); i != extensions.end(); i++ )
		std::cout << *i << "\n";

	std::shared_ptr<IExt1> e1 = extensionSystem.createExtension<IExt1>("Ext1");
	std::shared_ptr<IExt1> e2 = extensionSystem.createExtension<IExt1>("Ext1", 1000);
	std::shared_ptr<IExt2> e3 = extensionSystem.createExtension<IExt2>("Ext2");

	if(e1 != nullptr) {
		e1->test1();
		const ExtensionDescription *i = extensionSystem.findDescription(e1);
		if(i != nullptr)
			std::cout<<"Description:\n"<<*i<<std::endl;
	}
	if(e2 != nullptr) {
		e2->test1();
		const ExtensionDescription *i = extensionSystem.findDescription(e2);
		if(i != nullptr)
			std::cout<<"Description:\n"<<*i<<std::endl;
	}
	if(e3 != nullptr) {
		e3->test2();
		const ExtensionDescription *i = extensionSystem.findDescription(e3);
		if(i != nullptr)
			std::cout<<"Description:\n"<<*i<<std::endl;
	}

	std::cout<<"done"<<std::endl;

	return 0;
}
