/**
   @file
   @copyright
       Copyright Bernd Amend and Michael Adam 2014-2018
       Distributed under the Boost Software License, Version 1.0.
       (See accompanying file LICENSE_1_0.txt or copy at
       http://www.boost.org/LICENSE_1_0.txt)
*/
#pragma once

#include <string>
#include <functional>

namespace extension_system {

// Func = bool(const std::string&)
template <typename Func>
inline void split(const std::string& s, const char delimiter, Func func) {
    std::size_t pos      = 0;
    std::size_t last_pos = 0;
    do {
        pos = s.find(delimiter, last_pos);
        if (!func(s.substr(last_pos, pos - last_pos)))
            break;
        last_pos = pos + 1;
    } while (pos != std::string::npos);
}
}
