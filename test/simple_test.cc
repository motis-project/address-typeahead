#include <sstream>

#include "gtest/gtest.h"

#include "address-typeahead/common.h"
#include "address-typeahead/extractor.h"
#include "address-typeahead/typeahead.h"

using namespace address_typeahead;

class TestEnvironment : public testing::Environment {
public:
  TestEnvironment() : typeahead_(std::vector<std::string>()) {}

  virtual void SetUp() {
    extract("../test_resources/bremen.osm.pbf", context_);
    typeahead_ = typeahead(context_.get_all_place_names());
  }

  typeahead_context context_;
  typeahead typeahead_;
};

TestEnvironment* const test_env = static_cast<TestEnvironment*>(
    ::testing::AddGlobalTestEnvironment(new TestEnvironment));

TEST(Test, test_typeahead) {
  EXPECT_EQ("West", test_env->typeahead_.complete("test").at(0));
}

TEST(Test, test_get_area_names) {
  auto const& candidates = test_env->typeahead_.complete("test");
  auto const place = test_env->context_.get_place(candidates.at(5));
  auto const areas = test_env->context_.get_area_names_sorted(*place);
  EXPECT_EQ(2, areas.size());
  EXPECT_EQ("Mitte-Nord", areas.at(0));
}
