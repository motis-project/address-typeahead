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
  }

  auto postcodes = std::vector<index_t>();
  auto clean_strings = std::vector<std::string>();
  for (auto const& str : strings) {
    auto const val = atol(str.c_str());
    if (val == 0) {
      clean_strings.emplace_back(str);
    } else {
      postcodes.emplace_back(val);
    }
  }

  auto guess_strings = std::vector<std::string>();
  for (auto const& str : clean_strings) {
    if (str.length() >= 3) {
      guess_strings.emplace_back(str);
    }
  }
  if (options.string_chain_len_ > 1) {
    auto const start_i = options.first_string_is_place_ ? 1 : 0;
    for (size_t i = start_i; i + 1 < clean_strings.size(); ++i) {
      auto const chain_len =
          std::min(options.string_chain_len_,
                   static_cast<unsigned>(clean_strings.size() - i));
      auto str = clean_strings[i];
      for (size_t j = 1; j != chain_len; ++j) {
        str = str + " " + clean_strings[i + j];
      }
      if (str.length() >= 3) {
        guess_strings.emplace_back(str);
      }
    }
  }

  if (guess_strings.empty()) {
    auto result = std::vector<index_t>();
    for (auto const& pc : postcodes) {
      auto const pc_it = postcode_to_index_.find(pc);
      if (pc_it != postcode_to_index_.end()) {
        for (auto const& pc_idx : pc_it->second) {
          result.emplace_back(pc_idx);
        }
      }
    }
    if (result.size() > options.max_results_) {
      result.resize(options.max_results_);
    }
    return result;
  } else if (guess_strings.size() == 1 && postcodes.empty()) {
    auto guesses =
        place_guesser_.guess_match(guess_strings[0], options.max_results_);
    auto result = std::vector<index_t>();
    for (auto const& g : guesses) {
      if (g.cos_sim >= options.min_sim_) {
        for (auto const& p_idx : place_guess_to_index_[g.index]) {
          result.emplace_back(p_idx);
        }
      }
    }
    if (result.size() > options.max_results_) {
      result.resize(options.max_results_);
    }
    return result;
  }

  auto max_str_len = size_t(0);
  auto string_weights = std::vector<float>();
  for (auto const& str : guess_strings) {
    max_str_len = std::max(max_str_len, str.length());
    string_weights.emplace_back(str.length());
  }
  auto const normalization_val = 1.0f / static_cast<float>(max_str_len);
  for (size_t i = 0; i != string_weights.size(); ++i) {
    string_weights[i] = std::max(0.6f, string_weights[i] * normalization_val);
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
        max_cos_sim_area[ag.index] = std::max(max_cos_sim_area[ag.index],
                                              ag.cos_sim * string_weights[i]);
      }
    }
  } else {
    for (size_t i = 0; i != guess_strings.size(); ++i) {
      auto const& place_guesses =
          place_guesser_.guess_match(guess_strings[i], options.max_guesses_);
      for (auto const& pg : place_guesses) {
        max_cos_sim_place[pg.index] = std::max(max_cos_sim_place[pg.index],
                                               pg.cos_sim * string_weights[i]);
      }

      auto const& area_guesses =
          area_guesser_.guess_match(guess_strings[i], options.max_guesses_);
      for (auto const& ag : area_guesses) {
        max_cos_sim_area[ag.index] = std::max(max_cos_sim_area[ag.index],
                                              ag.cos_sim * string_weights[i]);
      }
    }
  }

  for (size_t i = 0; i != acc_.size(); ++i) {
    acc_[i] = std::pair<index_t, float>(i, 0.0f);
  }

  for (size_t i = 0; i != place_guess_to_index_.size(); ++i) {
    if (max_cos_sim_place[i] >= options.min_sim_) {
      for (auto const& place_idx : place_guess_to_index_[i]) {
        acc_[place_idx].second = std::max(
            acc_[place_idx].second, max_cos_sim_place[i] * options.place_bias_);
      }
    }
  }

  for (size_t i = 0; i != area_guess_to_index_.size(); ++i) {
    if (max_cos_sim_area[i] >= options.min_sim_) {
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
      std::begin(acc_), std::begin(acc_) + options.max_guesses_, std::end(acc_),
      [](auto const& lhs, auto const& rhs) { return lhs.second > rhs.second; });

  auto place_strings = std::vector<std::pair<std::string, float>>();
  auto index_translation_table = std::vector<index_t>();
  auto const i_max =
      std::min(static_cast<size_t>(options.max_guesses_), acc_.size());
  for (size_t i = 0; i != i_max; ++i) {
    place_strings.emplace_back(context_.get_name(acc_[i].first),
                               options.place_bias_);
    index_translation_table.emplace_back(i);

    auto num_of_postcode_matches = 0;
    auto const area_ids = context_.get_area_ids(acc_[i].first);
    for (auto const& area_id : area_ids) {
      auto const& a = context_.areas_[area_id];
      if (a.level_ == POSTCODE) {
        for (auto const& pc : postcodes) {
          if (pc == a.name_idx_) {
            ++num_of_postcode_matches;
          }
        }
        continue;
      }
      place_strings.emplace_back(context_.area_names_[a.name_idx_],
                                 a.popularity_);
      index_translation_table.emplace_back(i);
    }

    if (!postcodes.empty()) {
      acc_[i].second = static_cast<float>(num_of_postcode_matches) /
                       static_cast<float>(postcodes.size());
    } else {
      acc_[i].second = 0.0f;
    }
  }

  auto max_value = std::vector<float>(i_max);
  auto new_guesser = guess::guesser(place_strings);
  for (size_t str_i = 0; str_i != guess_strings.size(); ++str_i) {
    auto const& str = guess_strings[str_i];
    auto const guesses = new_guesser.guess_match(str, options.max_guesses_);
    std::fill(max_value.begin(), max_value.end(), 0.0f);
    for (auto const& g : guesses) {
      max_value[index_translation_table[g.index]] =
          std::max(max_value[index_translation_table[g.index]], g.cos_sim);
    }
    for (size_t i = 0; i != i_max; ++i) {
      acc_[i].second += max_value[i] * string_weights[str_i];
    }
  }

  std::sort(
      acc_.begin(), acc_.begin() + options.max_guesses_,
      [](auto const& lhs, auto const& rhs) { return lhs.second > rhs.second; });

  auto result = std::vector<index_t>();
  for (size_t i = 0; i != options.max_results_; ++i) {
    if (acc_[i].second >= options.min_sim_) {
      result.emplace_back(acc_[i].first);
    }
  }
  return result;
}

}  // namespace address_typeahead
