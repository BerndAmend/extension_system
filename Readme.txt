If you write an extension you only have to include extension_system/Extension.hpp (it will only depend on extension_system/macros.hpp), don't link against the ExtensionSystem.
If you want to load extensions you only need the file extension_system/ExtensionSystem.hpp.
