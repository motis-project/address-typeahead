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
    {
      std::ifstream in("../test_resources/out.map", std::ios::binary);
      cereal::BinaryInputArchive ia(in);
      ia(context_);
    }
    typeahead_ = typeahead(context_);
  }

  typeahead_context context_;
  typeahead typeahead_;
};

test_environment* const test_env = dynamic_cast<test_environment*>(
    ::testing::AddGlobalTestEnvironment(new test_environment));

TEST(Test, test_typeahead) {
  auto string_vec = std::vector<std::string>();
  string_vec.emplace_back("testc");
  auto const& result = test_env->typeahead_.complete(string_vec);

  EXPECT_EQ("Testcenter", test_env->context_.get_name(result.at(0)));
}

TEST(Test, test_get_area_names) {
  auto string_vec = std::vector<std::string>();
  string_vec.emplace_back("test");
  auto const& candidates = test_env->typeahead_.complete(string_vec);
  auto const areas = test_env->context_.get_area_names(candidates.at(5));
  EXPECT_EQ(5ul, areas.size());
  EXPECT_EQ("Bremen", areas.back().first);
}

TEST(Test, test_loading) {
  std::ifstream in("../test_resources/out.map", std::ios::binary);
  address_typeahead::typeahead_context context;
  {
    cereal::BinaryInputArchive ia(in);
    ia(context);
  }

  EXPECT_EQ(1728ul, context.streets_.size());
  EXPECT_EQ(17937ul, context.places_.size());

  auto t = typeahead(context);
  auto string_vec = std::vector<std::string>();
  string_vec.emplace_back("testce");
  auto candidates = t.complete(string_vec);
  auto area_names = context.get_area_names(candidates.at(0));
  EXPECT_EQ("Testcenter", context.get_name(candidates.at(0)));
  EXPECT_EQ(5ul, area_names.size());
  EXPECT_EQ("Bremen", area_names.back().first);

  string_vec.emplace_back("27568");
  candidates = t.complete(string_vec);
  EXPECT_EQ("Festma", context.get_name(candidates.at(1)));
}

TEST(Test, test_house_numbers) {
  auto string_vec = std::vector<std::string>();
  string_vec.emplace_back("gartenstr");
  string_vec.emplace_back("27568");
  auto const& result = test_env->typeahead_.complete(string_vec);

  auto const house_numbers = test_env->context_.get_house_numbers(result.at(0));
  ASSERT_TRUE(!house_numbers.empty());
  EXPECT_EQ("13", house_numbers[0]);

  double lat, lon;
  bool success = test_env->context_.coordinates_for_house_number(
      result.at(0), "13", lat, lon);
  ASSERT_TRUE(success);
  EXPECT_NEAR(53.5534, lat, 0.001);
  EXPECT_NEAR(8.57153, lon, 0.001);
}
