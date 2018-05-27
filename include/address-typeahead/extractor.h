#pragma once

#include "address-typeahead/common.h"

#include <osmium/tags/tags_filter.hpp>

namespace address_typeahead {

typeahead_context extract(std::string const& input_path,
                          osmium::TagsFilter const& filter, bool exact = false);

}  // namespace address_typeahead
