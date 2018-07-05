#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

#include <cereal/archives/binary.hpp>

#include "address-typeahead/common.h"
#include "address-typeahead/extractor.h"
#include "address-typeahead/serialization.h"
#include "address-typeahead/typeahead.h"

#include "timer.h"

std::string get_place_string(
    size_t id, address_typeahead::typeahead_context const& context) {
  std::string result = "";
  result += context.get_name(id) + " { ";
  auto const areas = context.get_area_names(id);
  for (auto const& a : areas) {
    result += a + ", ";
  }
  result += " }";

  auto house_numbers = context.get_house_numbers(id);
  std::sort(house_numbers.begin(), house_numbers.end());
  if (!house_numbers.empty()) {
    result += " { ";
    for (size_t i = 0; i != house_numbers.size(); ++i) {
      auto const& hn = house_numbers[i];
      if (!std::regex_match(hn, std::regex("\\d{1,4}[:alpha:]*")) ||
          (i + 1 != house_numbers.size() && house_numbers[i + 1] == hn)) {
        continue;
      }
      result += hn + ", ";
    }
    result += " }";
  }

  return result;
}

std::vector<address_typeahead::index_t> parse_string_and_complete(
    std::string const& str, address_typeahead::typeahead const& t,
    address_typeahead::typeahead_context const& context) {

  auto ss = std::stringstream(str);
  auto sub_strings = std::vector<std::string>();
  auto buf = std::string();

  while (ss >> buf) {
    sub_strings.emplace_back(buf);
  }

  auto house_number = std::string("");
  auto str_it = std::begin(sub_strings);
  while (str_it != std::end(sub_strings)) {
    if (std::regex_match(*str_it, std::regex("\\.|\\d{1,4}[:alpha:]*"))) {
      house_number = *str_it;
      str_it = sub_strings.erase(str_it);
    } else {
      ++str_it;
    }
  }

  if (sub_strings.size() == 0) {
    return std::vector<address_typeahead::index_t>();
  }

  address_typeahead::complete_options options;
  options.max_results_ = 10;
  options.string_chain_len_ = 2;
  auto const candidates = t.complete(sub_strings, options);

  if (house_number != "") {
    double lon, lat;
    if (house_number == ".") {
      if (context.get_coordinates(candidates[0], lat, lon)) {
        std::cout << "coordinates : " << lat << ", " << lon << std::endl;
      }
    } else {
      if (context.coordinates_for_house_number(candidates[0], house_number, lat,
                                               lon)) {
        std::cout << "coordinates : " << lat << ", " << lon << std::endl;
      } else {
        std::cout << "invalid house number" << std::endl;
      }
    }
  }

  return candidates;
}

void typeahead(std::string const& input_file) {
  auto in = std::ifstream(input_file, std::ios::binary);
  in.exceptions(std::ios_base::failbit);

  address_typeahead::typeahead_context context;
  {
    cereal::BinaryInputArchive ia(in);
    ia(context);
  }

  address_typeahead::typeahead t(context);

  std::string user_input;
  while (std::cout << "$ " && std::getline(std::cin, user_input)) {
    auto ti = address_typeahead::timer();
    auto candidates = parse_string_and_complete(user_input, t, context);

    for (auto const& c : candidates) {
      std::cout << get_place_string(c, context) << std::endl;
    }
    ti.elapsed_time_ms();
    std::cout << std::endl;
  }
}

void extract(std::string const& input_path, std::ofstream& out) {
  auto ti = address_typeahead::timer();

  address_typeahead::extract_options options;
  options.whitelist_add("name");
  options.whitelist_add("highway");
  options.whitelist_add("addr:street");
  options.whitelist_add("addr:housenumber");

  options.blacklist_add("tram");
  options.blacklist_add("power");
  options.blacklist_add("train");
  options.blacklist_add("aeroway");
  options.blacklist_add("waterway");
  options.blacklist_add("railway");
  options.blacklist_add("barrier");
  options.blacklist_add("service");
  options.blacklist_add("traffic_sign");
  options.blacklist_add("natural", "tree");
  options.blacklist_add("building", "garage");
  options.blacklist_add("amenity", "parking");
  options.blacklist_add("landuse", "garages");
  options.blacklist_add("landuse", "farmland");
  options.blacklist_add("highway", "service");
  options.blacklist_add("highway", "bus_stop");
  options.blacklist_add("amenity", "waste_disposal");

  char approx;
  std::cout << "approximate areas? (y/n) : ";
  std::cin >> approx;

  if (approx == 'y') {
    options.approximation_lvl_ = address_typeahead::APPROX_LVL_3;
  } else {
    options.approximation_lvl_ = address_typeahead::APPROX_NONE;
  }

  address_typeahead::typeahead_context context =
      address_typeahead::extract(input_path, options);
  ti.elapsed_time_s();

  {
    cereal::BinaryOutputArchive oa(out);
    oa(context);
  }
}

int main(int argc, char* argv[]) {
  if (argc == 4 && strcmp(argv[1], "extract") == 0) {
    std::ofstream out(argv[3], std::ios::binary);
    extract(argv[2], out);
  } else if (argc == 3 && strcmp(argv[1], "typeahead") == 0) {
    typeahead(argv[2]);
  } else {
    std::cout << "usage extract: " << argv[0] << " extract {input} {output}\n";
    std::cout << "usage typeahead: " << argv[0] << " typeahead {input}\n";
  }
}
