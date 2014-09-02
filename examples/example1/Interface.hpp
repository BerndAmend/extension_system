#pragma once

#include <extension_system/Extension.hpp>

class Interface1
{
public:
	virtual void test1() = 0;
	virtual ~Interface1() {}
};
EXTENSION_SYSTEM_INTERFACE(Interface1)
