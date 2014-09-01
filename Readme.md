# Extension System

## Overview

An extension contains:

* Implementation 
* Metadata:
    * Name
    * Description
    * Version number
    * User-specific metadata

### User-specific metadata
While developing extensions using Extension System, a user is able to export extension-specific metadata. 
This is data is encoded in a `key = value` style and can be used for example to 

* name a programme, the extension is designed for
* name the extensions license
* list extension authors
* etc.

There are no limits on user-specific metadata except that `key` and `value` have to be strings and that `\0` is prohibited in these strings.  

## Usage

### Developing an extension

For developing an extension one needs:

* An interface class marked as Extension System interface by `EXTENSION_SYSTEM_INTERFACE` macro
* A class that implements this interface exported by `EXTENSION_SYSTEM_EXTENSION` macro

Thus, one only needs to include `<extension_system/Extension.hpp>`. **No linking against Extension System libraries necessary.**

### Using extensions



## Limitations

* Extension System is unable to handle compressed shared libraries
* Extension System is unable to check extension's dependencies 