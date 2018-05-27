#include <cstring>
#include <fstream>
#include <iostream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "address-typeahead/common.h"
#include "address-typeahead/extractor.h"
#include "address-typeahead/serialization.h"
#include "address-typeahead/typeahead.h"

#include "timer.h"

std::string get_place_string(
    size_t id, address_typeahead::typeahead_context const& context) {
  std::string result = "";
  result += context.get_name(id) + " { ";
  auto const areas = context.get_area_names_sorted(id);
  for (auto const& a : areas) {
    result += a + ", ";
  }
  result += " }";
  return result;
}

std::vector<size_t> parse_string_and_complete(
    std::string const& str, address_typeahead::typeahead const& t,
    address_typeahead::typeahead_context const& context) {

  // expected string format :
  // name followed by areas (comma separated) and/or a house number
  // (indicated by :., :? or :integer)

  int state = 0;
  int str_begin = 0;

  char buffer[256] = {0};
  std::string name = "";
  std::vector<std::string> areas;
  std::string house_number = "";

  for (size_t i = 0; i != str.length(); ++i) {
    if (state == 0) {  // name
      if (str[i + 1] == '\0') {
        name = str;
      } else if (str[i + 1] == ',' || str[i + 1] == ':') {
        state = 3;
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, str.c_str(), i + 1 - str_begin);
        name = buffer;
      }
    } else if (state == 1) {  // area
      if (str[i + 1] == '\0' || str[i + 1] == ',' || str[i + 1] == ':') {
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, &str.c_str()[str_begin], i + 1 - str_begin);
        areas.push_back(std::string(buffer));
        state = 3;
      }
    } else if (state == 2) {  // house number
      if (str[i + 1] == '\0' || str[i + 1] == ',') {
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, &str.c_str()[str_begin], i + 1 - str_begin);
        house_number = buffer;
        state = 3;
      }
    } else {
      if (str[i] != ' ' && str[i] != ':' && str[i] != ',') {
        state = 1;
        str_begin = i;
      } else if (str[i] == ':') {
        state = 2;
        str_begin = i + 1;
      }
    }
  }

  auto const candidates = t.complete(name, areas);

  if (house_number != "") {
    if (house_number == "?") {
      auto const house_numbers = context.get_house_numbers(candidates[0]);
      if (!context.is_street(candidates[0])) {
        std::cout << "top match is not a street" << std::endl;
      } else {
        std::cout << "house numbers for top match : { ";
        for (auto const& hn : house_numbers) {
          std::cout << hn << ", ";
        }
        std::cout << " }" << std::endl;
      }
    } else if (house_number == ".") {
      double lon, lat;
      if (context.get_coordinates(candidates[0], lat, lon)) {
        std::cout << "coordinates : " << lat << ", " << lon << std::endl;
      }
    } else {
      double lon, lat;
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
  boost::archive::binary_iarchive ia(in);
  ia >> context;

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

  address_typeahead::typeahead_context context =
      address_typeahead::extract(input_path, filter, true);
  ti.elapsed_time_s();

  boost::archive::binary_oarchive oa(out);
  oa << context;
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
