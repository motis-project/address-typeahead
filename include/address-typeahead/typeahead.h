#pragma once

#include <string>
#include <vector>

#include <guess/guesser.h>

#include "common.h"

namespace address_typeahead {

struct typeahead {

  explicit typeahead(typeahead_context const context);

  std::vector<size_t> complete(std::string const& name,
                               std::vector<std::string> const& areas,
                               uint32_t const levels = 0xffffffff,
                               size_t max_results = 10) const;

private:
  typeahead_context context_;
  std::vector<std::string> names_;
  guess::guesser guesser_;
};

}  // namespace address_typeahead
