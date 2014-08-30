/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014
		Distributed under the Boost Software License, Version 1.0.
		(See accompanying file LICENSE_1_0.txt or copy at
		http://www.boost.org/LICENSE_1_0.txt)
*/
#pragma once

#include <extension_system/TypeSystem.hpp>

class IExt1
{
public:
	virtual void test1() = 0;
	virtual ~IExt1() {}
};

EXTENSION_SYSTEM_DECLARE_TYPE(IExt1, 'I','E','x','1')

class IExt2
{
public:
	virtual void test2() = 0;
	virtual ~IExt2() {}
};

EXTENSION_SYSTEM_DECLARE_TYPE(IExt2, 'I','E','x','2')
