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
  test_environment() : typeahead_(typeahead_context()) {}

  void SetUp() override {
    osmium::TagsFilter filter(true);
    filter.add_rule(false, "traffic_sign");
    filter.add_rule(false, "barrier");
    filter.add_rule(false, "aeroway");
    filter.add_rule(false, "waterway");
    filter.add_rule(false, "train");
    filter.add_rule(false, "tram");
    filter.add_rule(false, "railway");
    filter.add_rule(false, "amenity", "waste_disposal");
    filter.add_rule(false, "amenity", "parking");
    filter.add_rule(false, "landuse", "garages");

    extract("../test_resources/bremen.osm.pbf", context_, filter, true);
    typeahead_ = typeahead(context_);
  }

  typeahead_context context_;
  typeahead typeahead_;
};

test_environment* const test_env = dynamic_cast<test_environment*>(
    ::testing::AddGlobalTestEnvironment(new test_environment));

TEST(Test, test_typeahead) {
  auto const& result =
      test_env->typeahead_.complete("test", std::vector<std::string>());

  EXPECT_EQ("West", test_env->context_.get_name(result.at(0)));
}

TEST(Test, test_get_area_names) {
  auto const& candidates =
      test_env->typeahead_.complete("test", std::vector<std::string>());
  auto const areas = test_env->context_.get_area_names_sorted(candidates.at(5));
  EXPECT_EQ(2, areas.size());
  EXPECT_EQ("Mitte-Nord", areas.at(0));
}

TEST(Test, test_loading) {
  std::ifstream in("../test_resources/out.map", std::ios::binary);
  address_typeahead::typeahead_context context;
  boost::archive::binary_iarchive ia(in);
  ia >> context;

  EXPECT_EQ(1729, context.streets_.size());
  EXPECT_EQ(18409, context.places_.size());

  auto t = typeahead(context);
  auto areas = std::vector<std::string>();
  auto candidates = t.complete("test", areas);
  auto area_names = context.get_area_names_sorted(candidates.at(0));
  EXPECT_EQ("Testcenter", context.get_name(candidates.at(1)));
  EXPECT_EQ(5, area_names.size());
  EXPECT_EQ("Lehe", area_names.at(3));

  areas.push_back("nord");
  candidates = t.complete("test", areas);
  EXPECT_EQ("Vogelnest", context.get_name(candidates.at(2)));
}
