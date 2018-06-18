#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <guess/guesser.h>

#include "common.h"

namespace address_typeahead {

struct complete_options {
  bool first_string_is_place_ = false;

  // controls the influence places have on the result
  // < 1 favors areas; > 1 favors places
  float place_bias_ = 1.5f;

  float min_sim_ = 0.01f;

  unsigned max_guesses_ = 100;
  unsigned max_results_ = 10;

  // by using a string_chain_len_ > 1 the complete functions evaluates multiple
  // sequential strings together instead of evaluating each string in isolation
  unsigned string_chain_len_ = 1;
};

struct typeahead {

  explicit typeahead(typeahead_context const context);

  std::vector<index_t> complete(std::vector<std::string> const& strings,
                                size_t max_results = 10) const;

  std::vector<index_t> complete(std::vector<std::string> const& strings,
                                complete_options const& options) const;

  std::vector<std::vector<index_t>> place_guess_to_index_;
  std::vector<std::vector<index_t>> area_guess_to_index_;
  std::unordered_map<index_t, std::vector<index_t>> postcode_to_index_;

  typeahead_context context_;
  guess::guesser place_guesser_;
  guess::guesser area_guesser_;

private:
  mutable std::vector<std::pair<index_t, float>> acc_;
};

}  // namespace address_typeahead
