/**
	@file
	@copyright
		Copyright Bernd Amend and Michael Adam 2014
		Distributed under the Boost Software License, Version 1.0.
		(See accompanying file LICENSE_1_0.txt or copy at
		http://www.boost.org/LICENSE_1_0.txt)
*/
#pragma once

#include <string>
#include <vector>
#include <algorithm>

namespace extension_system {

	namespace {
		inline bool isWhitespace(const char c)
		{
			return c==' ' || c=='\n' || c=='\t' || c=='\r' || c==11;
		}
	}

	inline std::string trim(const std::string &s)
	{
		int left=0, right=s.length()-1;

		while(left<=right && isWhitespace(s[left]))
			++left;

		while(right>left && isWhitespace(s[right]))
			--right;

		return s.substr(left, 1+right-left);
	}

	inline std::vector<std::string> split(const std::string &s, char delimiter) {
		std::string token;
		std::vector<std::string> tokens;

		std::for_each(s.begin(), s.end(), [&](char c) {
			if (delimiter!=c) {
				token += c;
			} else {
				if (token.length())
					tokens.push_back(token);
				token.clear();
			}
		});
		if (token.length())
			tokens.push_back(token);
		return tokens;
	}
}
