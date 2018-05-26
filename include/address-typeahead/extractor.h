#pragma once

#include "address-typeahead/common.h"

#include <osmium/tags/tags_filter.hpp>

namespace address_typeahead {

void extract(std::string const& input_path, typeahead_context& context,
             osmium::TagsFilter const& filter, bool exact = false);

}  // namespace address_typeahead
