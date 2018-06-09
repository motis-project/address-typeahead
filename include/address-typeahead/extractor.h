#pragma once

#include "address-typeahead/common.h"

#include <osmium/tags/tags_filter.hpp>

uint32_t const APPROX_NONE(0);
uint32_t const APPROX_LVL_1(50000);
uint32_t const APPROX_LVL_2(100000);
uint32_t const APPROX_LVL_3(250000);
uint32_t const APPROX_LVL_4(500000);
uint32_t const APPROX_LVL_5(750000);
uint32_t const APPROX_MAX(999999);

namespace address_typeahead {

typeahead_context extract(std::string const& input_path,
                          osmium::TagsFilter const& filter,
                          uint32_t const approximation_lvl = APPROX_NONE);
}  // namespace address_typeahead
