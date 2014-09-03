#pragma once

#include <extension_system/Extension.hpp>

class Interface2
{
public:
	virtual void test1() = 0;
	virtual ~Interface2() {}
};
EXTENSION_SYSTEM_INTERFACE(Interface2)
