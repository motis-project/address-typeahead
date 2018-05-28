#include <fstream>
#include <sstream>

#include <gtest/gtest.h>

#include <cereal/archives/binary.hpp>

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

    context_ = extract("../test_resources/bremen.osm.pbf", filter, true);
    typeahead_ = typeahead(context_);
  }

  typeahead_context context_;
  typeahead typeahead_;
};

test_environment* const test_env = dynamic_cast<test_environment*>(
    ::testing::AddGlobalTestEnvironment(new test_environment));

TEST(Test, test_typeahead) {
  auto const& result =
      test_env->typeahead_.complete("testc", std::vector<std::string>());

  EXPECT_EQ("Testcenter", test_env->context_.get_name(result.at(0)));
}

TEST(Test, test_get_area_names) {
  auto const& candidates =
      test_env->typeahead_.complete("test", std::vector<std::string>());
  auto const areas = test_env->context_.get_area_names_sorted(candidates.at(5));
  EXPECT_EQ(5, areas.size());
  EXPECT_EQ("Bremen", areas.at(0));
}

TEST(Test, test_loading) {
  std::ifstream in("../test_resources/out.map", std::ios::binary);
  address_typeahead::typeahead_context context;
  {
    cereal::BinaryInputArchive ia(in);
    ia(context);
  }

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
  EXPECT_EQ("Yesterday", context.get_name(candidates.at(2)));
}

TEST(Test, test_house_numbers) {
  std::vector<std::string> areas;
  areas.emplace_back("nord");
  auto const& result = test_env->typeahead_.complete("gartenstr", areas);

  auto const house_numbers = test_env->context_.get_house_numbers(result.at(0));
  EXPECT_EQ("13", house_numbers[0]);

  double lat, lon;
  bool success = test_env->context_.coordinates_for_house_number(
      result.at(0), "13", lat, lon);
  ASSERT_TRUE(success);
  EXPECT_NEAR(53.5534, lat, 0.001);
  EXPECT_NEAR(8.57153, lon, 0.001);
}
