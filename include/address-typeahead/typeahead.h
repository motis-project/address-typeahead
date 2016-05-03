#pragma once

#include <string>
#include <vector>

#include "guess/guesser.h"

namespace address_typeahead {

struct typeahead {
  typeahead(std::string const& input_path);
  std::vector<std::string> complete(std::string const& user_input);

private:
  std::vector<std::string> names_;
  guess::guesser guesser_;
};

}  // namespace address-typeahead
