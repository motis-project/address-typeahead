#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>

#include "address-typeahead/common.h"
#include "address-typeahead/exporter.h"
#include "address-typeahead/extractor.h"
#include "address-typeahead/importer.h"
#include "address-typeahead/typeahead.h"
#include "address-typeahead/util.h"

void typeahead(std::string const& input_file) {
  std::ifstream in(input_file, std::ios::binary);
  in.exceptions(std::ios_base::failbit);

  address_typeahead::typeahead_context context;
  address_typeahead::binary_importer importer;
  importer.import_data(in, context);

  std::vector<std::string> lookup;
  for (auto const& pl : context.place_names_) {
    lookup.push_back(pl.second);
  }
  address_typeahead::typeahead t(lookup);

  std::string user_input;
  while (std::cout << "$ " && std::getline(std::cin, user_input)) {
    using std::chrono::system_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    auto start = system_clock::now();
    auto candidates = t.complete(user_input);
    auto duration = duration_cast<milliseconds>(system_clock::now() - start);
    std::cout << "\n\t" << duration.count() << "ms\n\n";

    for (auto const& c : candidates) {
      std::cout << "  " << c << " {";
      auto const place_id = address_typeahead::hash_string(c);
      address_typeahead::place p;
      p.name_id_ = place_id;
      auto const place = context.places_.find(p);
      auto const areas =
          address_typeahead::get_areas(place->coordinates_, context.areas_);
      for (auto const& a : areas) {
        auto const& ar = context.areas_[a];
        std::cout << ar.name_ << ", ";
      }
      std::cout << "}\n";
    }
    std::cout << "\n";
  }
}

void extract(std::string const& input_path, std::ofstream& out) {
  auto ti = address_typeahead::timer();
  address_typeahead::typeahead_context context;
  address_typeahead::extract(input_path, context);
  ti.elapsed_time_s();

  address_typeahead::binary_exporter exporter;
  exporter.export_data(out, context);
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
