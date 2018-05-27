#include "address-typeahead/typeahead.h"

#include <algorithm>

using namespace guess;

namespace address_typeahead {

typeahead::typeahead(typeahead_context const context)
    : context_(context), guesser_(context_.get_all_names_weighted()) {}

std::vector<size_t> typeahead::complete(std::string const& name,
                                        std::vector<std::string> const& areas,
                                        uint32_t const levels,
                                        size_t max_results) const {
  auto matches = guesser_.guess_match(name, 400);

  if (areas.empty()) {
    auto guesses = std::vector<size_t>();
    for (size_t i = 0; i != std::min(max_results, matches.size()); ++i) {
      auto const& match = matches[i];
      guesses.push_back(match.index);
    }
    return guesses;
  }

  std::vector<size_t> area_match_to_match;
  std::vector<std::pair<std::string, float>> area_names;
  for (size_t i = 0; i != matches.size(); ++i) {
    auto const a_names =
        context_.get_area_names_weighted(matches[i].index, levels);
    area_names.insert(area_names.end(), a_names.begin(), a_names.end());
    area_match_to_match.insert(area_match_to_match.end(), a_names.size(), i);
  }

  guess::guesser gsr(area_names);
  for (auto const& area_name : areas) {
    auto const area_matches = gsr.guess_match(area_name, 100);
    for (auto const& area_match : area_matches) {
      auto& match = matches[area_match_to_match[area_match.index]];
      match.cos_sim += area_match.cos_sim / areas.size();
    }
  }

  std::sort(std::begin(matches), std::end(matches));

  auto guesses = std::vector<size_t>();
  for (size_t i = 0; i != std::min(max_results, matches.size()); ++i) {
    auto const& match = matches[i];
    guesses.push_back(match.index);
  }
  return guesses;
}

}  // namespace address_typeahead
