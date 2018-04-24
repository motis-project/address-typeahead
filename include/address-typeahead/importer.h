#pragma once

#include <iostream>

#include "common.h"

namespace address_typeahead {

class importer {
public:
  virtual void import_data(std::istream& in, typeahead_context& data) = 0;
};

class binary_importer : public importer {
public:
  void import_data(std::istream& in, typeahead_context& data);
};

}  // namespace address_typeahead
