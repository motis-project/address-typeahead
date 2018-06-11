#pragma once

#include "address-typeahead/common.h"

#include <osmium/tags/tags_filter.hpp>

namespace address_typeahead {

uint32_t const APPROX_NONE(0);
uint32_t const APPROX_LVL_1(50000);
uint32_t const APPROX_LVL_2(100000);
uint32_t const APPROX_LVL_3(250000);
uint32_t const APPROX_LVL_4(500000);
uint32_t const APPROX_LVL_5(750000);
uint32_t const APPROX_MAX(999999);

struct extract_options {
  uint32_t approximation_lvl_ = APPROX_NONE;
  osmium::TagsFilter whitelist_ = osmium::TagsFilter(false);
  osmium::TagsFilter blacklist_ = osmium::TagsFilter(false);

  void whitelist_add(std::string const& tag, std::string const& value = "");
  void blacklist_add(std::string const& tag, std::string const& value = "");
};

typeahead_context extract(std::string const& input_path,
                          extract_options const& options);
}  // namespace address_typeahead
