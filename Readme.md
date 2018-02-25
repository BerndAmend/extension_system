# Extension System [![Build Status](https://travis-ci.org/tptb/extension_system.svg?branch=master)](https://travis-ci.org/tptb/extension_system) [![Build status](https://ci.appveyor.com/api/projects/status/5cydq9lah0bj2d0m/branch/master?svg=true)](https://ci.appveyor.com/project/tptb/extension-system/branch/master)

## Overview

Extension System is a library to allow programmers to efficiently extend their programs (plugin system).
Extension System is highly efficient and can deal with hundreds or thousands of extensions. In difference to other plugin systems Extension System **does not** load the extension libraries while scanning for extensions. Extension System parses the shared libraries and searches for extensions within them.

During scanning for extension, following metadata is extracted for each extension:

* Name (text)
* Description (text)
* Version number (integer)
* Interface name (text)
* Compiler (text)
* Compiler-Version (text)
* Build type (text, "release"/"debug")
* User-specific metadata

Extension System provides access to all this metadata.

In order to instantiate an extension, a user has to provide:

interface and name

    std::shared_ptr<Interface1> extension = extensionSystem.createExtension<Interface1>("Extension1");

or interface, name and specific version

    std::shared_ptr<Interface1> extension = extensionSystem.createExtension<Interface1>("Extension1", 3);

If no version is given, the highest available version will be instantiated.
When an extension is instantiated, the related shared library is loaded. Extension system tracks references to shared libraries (how many extensions from this library are currently alive). If no reference is left the shared library will automatically be unloaded.

### Supported Platforms

* Linux using GCC >=4.8
* Linux using Clang/LLVM >= 3.4
* Windows using Visual Studio >=2015
* Windows using Mingw-w64 >=4.8
* OS X using Apple LLVM version 9.0.0

### User-specific metadata
While developing extensions using Extension System, a user is able to export extension-specific metadata.
This is data is encoded in a `key = value` style and can be used for example to

* name a programme, the extension is designed for
* name the extensions license
* list extension authors
* etc.

There are no limitations on user-specific metadata except that `key` and `value` have to be strings and that `\0` is prohibited in these strings.

## Usage

### Developing an extension

For developing an extension one needs:

* An interface class marked as Extension System interface by `EXTENSION_SYSTEM_INTERFACE` macro
* A class that implements this interface exported by `EXTENSION_SYSTEM_EXTENSION` macro

Thus, one only needs to include `<extension_system/Extension.hpp>`. **No linking against Extension System libraries necessary.**

### Using extensions

\TODO

## Example

Interface.hpp
```C++
#pragma once

#include <extension_system/Extension.hpp>

class Interface1
{
public:
    virtual void test1() = 0;
    virtual ~Interface1() {}
};
EXTENSION_SYSTEM_INTERFACE(Interface1)
```

Extension1.cpp
```C++
#include "Interface.hpp"
#include <iostream>

class Extension1 : public Interface1
{
public:
    virtual void test1() override {
        std::cout<<"Hello from Extension1"<<std::endl;
    }
};
EXTENSION_SYSTEM_EXTENSION(Interface1, Extension1, "Extension1", 100, "extension 1 for testing purposes (Version 100)", "")
```

main.cpp
```C++
#include "Interface.hpp"
#include <iostream>

int main() {
    extension_system::ExtensionSystem extensionSystem;
    extensionSystem.searchDirectory("./");
    auto e1 = extensionSystem.createExtension<Interface1>("Extension1");
    if(e1 != nullptr)
        e1->test1();
    std::cout<<"Done."<<std::endl;
    return 0;
}
```

## Limitations

* Extension System is unable to handle compressed shared libraries
* Extension System is unable to check extension's dependencies

## License
Extension System is licensed under [BSL 1.0 (Boost Software License 1.0)](LICENSE_1_0.txt)
