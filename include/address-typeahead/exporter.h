#pragma once

#include <iostream>

#include "common.h"

namespace address_typeahead {

class exporter {
public:
  virtual void export_data(std::ostream& out,
                           typeahead_context const& data) = 0;
};

class binary_exporter : public exporter {
public:
  void export_data(std::ostream& out, typeahead_context const& data);
};

}  // namespace address_typeahead
