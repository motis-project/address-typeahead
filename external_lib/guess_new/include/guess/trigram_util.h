#pragma once

#include <string>

namespace guess {

using compressed_trigram = uint16_t;

constexpr auto trigram_char_bit_width = 5;

constexpr compressed_trigram to_5_bit(uint8_t c) {
  return c >= 'a' && c <= 'z' ? c - 'a' : 25;
}

constexpr compressed_trigram compress_trigram(char const* str) {
  return (to_5_bit(str[0]) << (0 * trigram_char_bit_width)) +
         (to_5_bit(str[1]) << (1 * trigram_char_bit_width)) +
         (to_5_bit(str[2]) << (2 * trigram_char_bit_width));
}

constexpr auto max_compressed_trigram = compress_trigram("zzz");

template <typename Fn>
void for_each_trigram(std::string const& s, Fn fn) {
  if (s.length() < 3) {
    return;
  }

  auto substr = s.c_str();
  while (substr[2] != '\0') {
    fn(compress_trigram(substr));
    ++substr;
  }
}

}  // namespace guess
