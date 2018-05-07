#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "address-typeahead/common.h"
#include "address-typeahead/extractor.h"
#include "address-typeahead/serialization.h"
#include "address-typeahead/timer.h"
#include "address-typeahead/typeahead.h"

void typeahead(std::string const& input_file) {
  std::ifstream in(input_file, std::ios::binary);
  in.exceptions(std::ios_base::failbit);

  address_typeahead::typeahead_context context;
  boost::archive::binary_iarchive ia(in);
  ia >> context;

  auto const lookup = context.get_all_place_names();
  address_typeahead::typeahead t(lookup);

  std::string user_input;
  while (std::cout << "$ " && std::getline(std::cin, user_input)) {
    using std::chrono::system_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    auto start = system_clock::now();
    auto candidates = t.complete(user_input);
    auto duration = duration_cast<milliseconds>(system_clock::now() - start);

    for (auto const& c : candidates) {
      std::cout << "  " << c << " {";
      auto const place = context.get_place(c);
      auto const areas = context.get_area_names_sorted(*place);
      for (auto const& a : areas) {
        std::cout << a << ", ";
      }
      std::cout << "} \n";
    }
    std::cout << duration.count() << "ms\n\n";
    std::cout << "\n";
  }
}

void extract(std::string const& input_path, std::ofstream& out) {
  auto ti = address_typeahead::timer();
  address_typeahead::typeahead_context context;
  address_typeahead::extract(input_path, context);
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
