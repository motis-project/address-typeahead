#include "address-typeahead/typeahead.h"

#include <algorithm>

using namespace guess;

namespace address_typeahead {

std::vector<std::pair<std::string, float>> get_names(
    typeahead_context const& context, bool const areas) {
  if (areas) {
    auto result = std::vector<std::pair<std::string, float>>();
    result.reserve(context.areas_.size());
    for (auto const& a : context.areas_) {
      if (a.level_ != POSTCODE) {
        result.emplace_back(context.area_names_[a.name_idx_], a.popularity_);
      } else {
        result.emplace_back("", 0.0f);
      }
    }
    return result;
  }

  auto result = std::vector<std::pair<std::string, float>>();
  result.reserve(context.names_.size());
  for (auto const& name : context.names_) {
    result.emplace_back(name, 1.0f);
  }
  return result;
}

typeahead::typeahead(typeahead_context const context)
    : context_(context),
      place_guesser_(get_names(context_, false)),
      area_guesser_(get_names(context_, true)) {

  auto const i_max = context_.places_.size() + context_.streets_.size();
  acc_.resize(i_max);
  place_guess_to_index_.resize(place_guesser_.candidates_.size());
  area_guess_to_index_.resize(area_guesser_.candidates_.size());

  for (index_t i = 0; i != i_max; ++i) {
    place_guess_to_index_[context_.get_name_id(i)].emplace_back(i);

    auto const area_ids = context_.get_area_ids(i);
    for (auto const& area_id : area_ids) {
      auto const& a = context_.areas_[area_id];
      if (a.level_ != POSTCODE) {
        area_guess_to_index_[area_id].emplace_back(i);
      } else {
        auto postcode_it = postcode_to_index_.find(a.name_idx_);
        if (postcode_it == postcode_to_index_.end()) {
          postcode_it =
              postcode_to_index_.emplace(a.name_idx_, std::vector<index_t>())
                  .first;
        }
        postcode_it->second.emplace_back(i);
      }
    }
  }
}

std::vector<index_t> typeahead::complete(
    std::vector<std::string> const& strings, size_t max_results) const {
  complete_options options;
  options.max_results_ = max_results;
  return complete(strings, options);
}

std::vector<index_t> typeahead::complete(
    std::vector<std::string> const& strings,
    complete_options const& options) const {
  if (strings.empty()) {
    return std::vector<index_t>();
  } else if (strings.size() == 1) {
    auto guesses = place_guesser_.guess(strings[0], options.max_results_);
    auto result = std::vector<index_t>();
    for (auto const& g : guesses) {
      for (auto const& p_idx : place_guess_to_index_[g]) {
        result.emplace_back(p_idx);
      }
    }
    result.resize(options.max_results_);
    return result;
  }

  auto clean_strings = std::vector<std::string>();
  auto postcodes = std::vector<index_t>();
  for (auto const& str : strings) {
    auto const val = atol(str.c_str());
    if (val == 0) {
      clean_strings.emplace_back(str);
    } else {
      postcodes.emplace_back(val);
    }
  }

  auto guess_strings = std::vector<std::string>();
  guess_strings.insert(guess_strings.begin(), clean_strings.begin(),
                       clean_strings.end());
  if (options.string_chain_len_ > 1) {
    auto const start_i = options.first_string_is_place_ ? 1 : 0;
    for (size_t i = start_i; i != clean_strings.size() - 1; ++i) {
      auto const chain_len =
          std::min(options.string_chain_len_,
                   static_cast<unsigned>(clean_strings.size() - i));
      auto str = clean_strings[i];
      for (size_t j = 1; j != chain_len; ++j) {
        str = str + " " + clean_strings[i + j];
      }
      guess_strings.emplace_back(str);
    }
  }

  auto max_cos_sim_place = std::vector<float>(place_guess_to_index_.size());
  auto max_cos_sim_area = std::vector<float>(area_guess_to_index_.size());
  std::fill(max_cos_sim_place.begin(), max_cos_sim_place.end(), 0.0f);
  std::fill(max_cos_sim_area.begin(), max_cos_sim_area.end(), 0.0f);

  if (options.first_string_is_place_) {
    auto const& place_guesses =
        place_guesser_.guess_match(guess_strings[0], options.max_guesses_);
    for (auto const& pg : place_guesses) {
      max_cos_sim_place[pg.index] =
          std::max(max_cos_sim_place[pg.index], pg.cos_sim);
    }

    for (size_t i = 1; i != guess_strings.size(); ++i) {
      auto const& area_guesses =
          area_guesser_.guess_match(guess_strings[i], options.max_guesses_);
      for (auto const& ag : area_guesses) {
        max_cos_sim_area[ag.index] =
            std::max(max_cos_sim_area[ag.index], ag.cos_sim);
      }
    }
  } else {
    for (size_t i = 0; i != guess_strings.size(); ++i) {
      auto const& place_guesses =
          place_guesser_.guess_match(guess_strings[i], options.max_guesses_);
      for (auto const& pg : place_guesses) {
        max_cos_sim_place[pg.index] =
            std::max(max_cos_sim_place[pg.index], pg.cos_sim);
      }

      auto const& area_guesses =
          area_guesser_.guess_match(guess_strings[i], options.max_guesses_);
      for (auto const& ag : area_guesses) {
        max_cos_sim_area[ag.index] =
            std::max(max_cos_sim_area[ag.index], ag.cos_sim);
      }
    }
  }

  for (size_t i = 0; i != acc_.size(); ++i) {
    acc_[i] = std::pair<index_t, float>(i, 0.0f);
  }

  auto const threshold = 0.05f;
  for (size_t i = 0; i != place_guess_to_index_.size(); ++i) {
    if (max_cos_sim_place[i] > threshold) {
      for (auto const& place_idx : place_guess_to_index_[i]) {
        acc_[place_idx].second = std::max(
            acc_[place_idx].second, max_cos_sim_place[i] * options.place_bias_);
      }
    }
  }

  for (size_t i = 0; i != area_guess_to_index_.size(); ++i) {
    if (max_cos_sim_area[i] > threshold) {
      for (auto const& area_idx : area_guess_to_index_[i]) {
        acc_[area_idx].second += max_cos_sim_area[i];
      }
    }
  }

  for (auto const& pc : postcodes) {
    auto const pc_it = postcode_to_index_.find(pc);
    if (pc_it != postcode_to_index_.end()) {
      for (auto const& pc_idx : pc_it->second) {
        acc_[pc_idx].second += 1.0f;
      }
    }
  }

  std::nth_element(
      std::begin(acc_), std::begin(acc_) + options.max_results_, std::end(acc_),
      [](auto const& lhs, auto const& rhs) { return lhs.second > rhs.second; });
  std::sort(
      acc_.begin(), acc_.begin() + options.max_results_,
      [](auto const& lhs, auto const& rhs) { return lhs.second > rhs.second; });

  auto result = std::vector<index_t>();
  for (size_t i = 0; i != options.max_results_; ++i) {
    result.emplace_back(acc_[i].first);
  }

  return result;
}

}  // namespace address_typeahead
