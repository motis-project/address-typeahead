#pragma once

#include <string>

namespace guess {

template <typename Function>
inline void for_each_token(char* s, std::size_t length, Function f) {
  auto base = 0;
  auto i = 0u;
  while (i < length) {
    if (s[i] == ' ') {
      char tmp = s[i];
      s[i] = '\0';
      bool exit = f(s + base);
      s[i] = tmp;

      if (exit) {
        return;
      }

      base = i + 1;
    }
    ++i;
  }
  f(s + base);
}

template <typename Function>
inline void for_each_token(std::string const& in, Function f) {
  for_each_token(const_cast<char*>(in.c_str()), in.size(), f);
}

inline void replace_all(std::string& s, std::string const& from,
                        std::string const& to) {
  std::string::size_type pos;
  while ((pos = s.find(from)) != std::string::npos) {
    s.replace(pos, from.size(), to);
  }
}

inline void normalize(std::string& s) {
  replace_all(s, "è", "e");
  replace_all(s, "é", "e");
  replace_all(s, "Ä", "a");
  replace_all(s, "ä", "a");
  replace_all(s, "Ö", "o");
  replace_all(s, "ö", "o");
  replace_all(s, "Ü", "u");
  replace_all(s, "ü", "u");
  replace_all(s, "ß", "ss");
  replace_all(s, "-", " ");
  replace_all(s, "/", " ");
  replace_all(s, ".", " ");
  replace_all(s, ",", " ");
  replace_all(s, "(", " ");
  replace_all(s, ")", " ");

  for (int i = 0; i < s.length(); ++i) {
    char c = s[i];
    bool is_number = c >= '0' && c <= '9';
    bool is_lower_case_char = c >= 'a' && c <= 'z';
    bool is_upper_case_char = c >= 'A' && c <= 'Z';
    if (!is_number && !is_lower_case_char && !is_upper_case_char) {
      s[i] = ' ';
    }
  }

  replace_all(s, "  ", " ");

  std::transform(begin(s), end(s), begin(s), ::tolower);
}

}  // namespace guess
