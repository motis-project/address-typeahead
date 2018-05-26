#include "address-typeahead/typeahead.h"

#include <algorithm>

using namespace guess;

namespace address_typeahead {

typeahead::typeahead(typeahead_context const context)
    : context_(context), names_(context_.get_all_names()), guesser_(names_) {}

std::vector<size_t> typeahead::complete(std::string const& name,
                                        std::vector<std::string> const& areas,
                                        uint32_t const levels,
                                        size_t max_results) const {
  auto const max_matches = size_t(400);

  auto matches = guesser_.guess_threshold(name, 0.1, 10);
  matches.resize(std::min(matches.size(), max_matches));

  if (areas.empty()) {
    auto guesses = std::vector<size_t>();
    for (size_t i = 0; i != std::min(max_results, matches.size()); ++i) {
      auto const& match = matches[i];
      guesses.push_back(match.index_);
    }
    return guesses;
  }

  auto const w1 = double(0.5);
  auto const w2 = double(1.0 - w1);
  for (auto& match : matches) {
    auto const area_names = context_.get_area_names(match.index_, levels);
    guess::guesser gsr(area_names);

    auto sum = double(0);
    for (auto const& a : areas) {
      auto const area_matches = gsr.guess_threshold(a, 0, 10);
      if (!area_matches.empty()) {
        sum += area_matches[0].cos_sim_;
      }
    }

    match.cos_sim_ = match.cos_sim_ * w1 + (sum / areas.size()) * w2;
  }

  std::sort(std::begin(matches), std::end(matches));

  auto guesses = std::vector<size_t>();
  for (size_t i = 0; i != std::min(max_results, matches.size()); ++i) {
    auto const& match = matches[i];
    guesses.push_back(match.index_);
  }
  return guesses;
}

}  // namespace address_typeahead
