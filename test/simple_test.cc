#include <fstream>
#include <sstream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "gtest/gtest.h"

#include "address-typeahead/common.h"
#include "address-typeahead/extractor.h"
#include "address-typeahead/serialization.h"
#include "address-typeahead/typeahead.h"

using namespace address_typeahead;

class test_environment : public testing::Environment {
public:
  test_environment() : typeahead_(std::vector<std::string>()) {}

  void SetUp() override {
    extract("../test_resources/bremen.osm.pbf", context_);
    typeahead_ = typeahead(context_.get_all_place_names());
  }

  typeahead_context context_;
  typeahead typeahead_;
};

test_environment* const test_env = dynamic_cast<test_environment*>(
    ::testing::AddGlobalTestEnvironment(new test_environment));

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

TEST(Test, test_loading) {
  std::ifstream in("../test_resources/out.map", std::ios::binary);
  address_typeahead::typeahead_context context;
  boost::archive::binary_iarchive ia(in);
  ia >> context;

  auto const lookup = context.get_all_place_names();
  EXPECT_EQ(17937, lookup.size());

  auto t = typeahead(context.get_all_place_names());
  auto const& candidates = t.complete("test");
  auto const place = context.get_place(candidates.at(0));
  auto const areas = context.get_area_names_sorted(*place);
  EXPECT_EQ(8, areas.size());
  EXPECT_EQ("Hemelingen", areas.at(3));
}
