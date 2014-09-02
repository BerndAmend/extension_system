#include <extension_system/Extension.hpp>
#include "Interface.hpp"
#include <iostream>

class Extension1 : public Interface1
{
public:
	virtual void test1() override {
		std::cout<<"Hello from Extension1"<<std::endl;
	}
};

EXTENSION_SYSTEM_EXTENSION(Interface1, Extension1, "Extension1", 100, "extension 1 for testing purposes (Version 100)", EXTENSION_SYSTEM_NO_USER_DATA)
