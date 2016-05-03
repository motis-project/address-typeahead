#include <sstream>

#include "gtest/gtest.h"

#include "address-typeahead/extractor.h"
#include "address-typeahead/typeahead.h"

using namespace address_typeahead;

TEST(simple, simple) {
  std::stringstream out;
  extract("../test_resources/map.osm", out);
  EXPECT_EQ("Hochschulstraße", typeahead(out).complete("Hoch").at(0));
}
