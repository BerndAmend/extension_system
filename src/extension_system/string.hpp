/// SPDX-FileCopyrightText: 2014-2020 Bernd Amend and Michael Adam
/// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <string>
#include <functional>

namespace extension_system {

/// @returns true=the string was completely splitted, false=the func aborted the splitting
// Func = bool(const std::string&)
template <typename Func>
inline bool split(const std::string& s, const char delimiter, Func func) {
    std::size_t pos      = 0;
    std::size_t last_pos = 0;
    do {
        pos = s.find(delimiter, last_pos);
        if (!func(s.substr(last_pos, pos - last_pos)))
            return false;
        last_pos = pos + 1;
    } while (pos != std::string::npos);
    return true;
}
}
