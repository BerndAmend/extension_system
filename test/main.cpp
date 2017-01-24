/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014
		Distributed under the Boost Software License, Version 1.0.
		(See accompanying file LICENSE_1_0.txt or copy at
		http://www.boost.org/LICENSE_1_0.txt)
*/
#include "Interfaces.hpp"
#include <extension_system/ExtensionSystem.hpp>
#include <iostream>

using namespace extension_system;

static std::shared_ptr<IExt1> e1;

int main() {
	ExtensionSystem extensionSystem;

	extensionSystem.searchDirectory(".");

	for(const auto &i : extensionSystem.extensions())
		std::cout << i.toString() << "\n";

	std::cout << "\nExtensions with interface IExt1:\n";
	for(const auto &i : extensionSystem.extensions<IExt1>())
		std::cout << i.toString() << "\n";

	e1 = extensionSystem.createExtension<IExt1>("Ext1");
	auto e2 = extensionSystem.createExtension<IExt1>("Ext1", 100);
	auto e3 = extensionSystem.createExtension<IExt2>("Ext2");

	if(e1 != nullptr) {
		e1->test1();
		auto i = extensionSystem.findDescription(e1);
		if(i.isValid())
			std::cout<<"Description:\n"<<i.toString()<<"\n";
	}
	if(e2 != nullptr) {
		e2->test1();
		auto i = extensionSystem.findDescription(e2);
		if(i.isValid())
			std::cout<<"Description:\n"<<i.toString()<<"\n";
	}
	if(e3 != nullptr) {
		e3->test2();
		auto i = extensionSystem.findDescription(e3);
		if(i.isValid())
			std::cout<<"Description:\n"<<i.toString()<<"\n";
	}

	std::cout<<"done\n";

	std::cout<<"filter test\n";
	auto filteredExtensions = extensionSystem.extensions({{"Test1", "desc1"}, {"Test1", "desc2"}, {"Test3", "desc3"}});
	for(const auto &i : filteredExtensions) {
		auto e4 = extensionSystem.createExtension<IExt2>(i);
		if(e4 != nullptr) {
			std::cout << i.toString() << "\n";
			e4->test2();
		} else {
			std::cout << "Wrong interface:\n" << i.toString() << "\n";
		}
	}

	return 0;
}
