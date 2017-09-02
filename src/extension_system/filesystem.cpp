/**
    @file
    @copyright
        Copyright Bernd Amend and Michael Adam 2014-2017
        Distributed under the Boost Software License, Version 1.0.
        (See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt)
*/
#include <extension_system/filesystem.hpp>

#ifdef EXTENSION_SYSTEM_USE_STD_FILESYSTEM
using namespace extension_system::filesystem;

#if EXTENSION_SYSTEM_COMPILER_VERSION >= 1700
path extension_system::filesystem::canonical(const path& p)
{
    return p.filename();
}
#endif

void extension_system::filesystem::forEachFileInDirectory(const path& root, const std::function<void(const path& p)>& func, bool recursive)
{
    if (!exists(root) && !is_directory(root)) {
        return;
    }

    const auto options = directory_options::skip_permission_denied | directory_options::follow_directory_symlink;

    if (recursive) {
        recursive_directory_iterator end_iter;
        for (recursive_directory_iterator dir_iter(root, options); dir_iter != end_iter; ++dir_iter) {
            func(dir_iter->path());
        }
    } else {
        directory_iterator end_iter;
        for (directory_iterator dir_iter(root, options); dir_iter != end_iter; ++dir_iter) {
            func(dir_iter->path());
        }
    }
}

#else

#include <climits>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

bool extension_system::filesystem::exists(const extension_system::filesystem::path& p)
{
    const std::string str = p.string();
    return access(str.c_str(), R_OK) == 0;
}

bool extension_system::filesystem::is_directory(const extension_system::filesystem::path& p)
{
    const std::string str = p.string();
    // clang-format off
    struct stat       sb {};
    // clang-format on
    return (stat(str.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}

extension_system::filesystem::path extension_system::filesystem::canonical(const extension_system::filesystem::path& p)
{
#ifdef EXTENSION_SYSTEM_COMPILER_MINGW
    return p;
#else
    std::array<char, PATH_MAX> buffer{};
    const std::string path   = p.string();
    const char*       result = realpath(path.c_str(), buffer.data());
    if (result == nullptr) {
        return p; // couldn't resolve symbolic link
    }
    return std::string(result);
#endif
}

void extension_system::filesystem::forEachFileInDirectory(const extension_system::filesystem::path& root,
                                                          const std::function<void(const extension_system::filesystem::path& p)>& func,
                                                          bool                                                                    recursive)
{
    const std::function<void(const extension_system::filesystem::path& p)> handle_dir
        = [&func, &recursive, &handle_dir](const extension_system::filesystem::path& p) {

              const std::string path_string = p.string();
              DIR*              dp          = opendir(path_string.c_str());

              if (dp != nullptr) {
                  dirent* ep = nullptr;
                  while ((ep = readdir(dp)) != nullptr) {
                      const std::string      name{ep->d_name};
                      const filesystem::path full_name = p / name;

                      if (name == "." || name == "..") {
                          continue;
                      }

                      if (ep->d_type == DT_REG || ep->d_type == DT_LNK) {
                          if (recursive && is_directory(full_name)) {
                              handle_dir(full_name);
                          } else {
                              func(full_name);
                          }
                      } else if (recursive && ep->d_type == DT_DIR) {
                          handle_dir(full_name);
                      }
                  }

                  (void)closedir(dp);
              } else {
                  // not a directory???
              }
          };

    handle_dir(root);
}

#endif
