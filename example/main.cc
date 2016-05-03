#include <chrono>
#include <cstring>
#include <iostream>

#include "address-typeahead/extractor.h"
#include "address-typeahead/typeahead.h"

void typeahead(std::string const& input_file) {
  address_typeahead::typeahead t(input_file);

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
      std::cout << "  " << c << "\n";
    }
    std::cout << "\n";
  }
}

int main(int argc, char* argv[]) {
  if (argc == 4 && strcmp(argv[1], "extract") == 0) {
    address_typeahead::extract(argv[2], argv[3]);
  } else if (argc == 3 && strcmp(argv[1], "typeahead") == 0) {
    typeahead(argv[2]);
  } else {
    std::cout << "usage extract: " << argv[0] << " extract {input} {output}\n";
    std::cout << "usage typeahead: " << argv[0] << " typeahead {input}\n";
  }
}
