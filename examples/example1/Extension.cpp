#include <extension_system/Extension.hpp>
#include "Interface.hpp"
#include <iostream>

class Extension : public Interface1
{
public:
	virtual void test1() override {
		std::cout<<"Hello from Extension1"<<std::endl;
	}
};

EXTENSION_SYSTEM_EXTENSION(Interface1, Extension, "Example1Extension", 100, "Example 1 extension", EXTENSION_SYSTEM_NO_USER_DATA)
