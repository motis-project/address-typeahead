#ifndef GUESS_STRING_UTIL_H_
#define GUESS_STRING_UTIL_H_

#include <string>

namespace guess {

template <typename Function>
inline void for_each_token(char* s, std::size_t length, Function f) {
  int base = 0;
  int i = 0;
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

void replace_all(std::string& s,
                 std::string const& from,
                 std::string const& to) {
  std::string::size_type pos;
  while ((pos = s.find(from)) != std::string::npos) {
    s.replace(pos, from.size(), to);
  }
}

}  // namespace guess

#endif  // GUESS_STRING_UTIL_H_