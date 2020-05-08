#pragma once

#include <fstream>
#include <ostream>
#include <string>

namespace address_typeahead {

void extract(std::string const& input_path, std::ofstream& out);

}  // namespace address_typeahead