#include <iostream>
#include <fstream>
#include <chrono>

#include "guess/guesser.h"

using namespace guess;

struct new_line_tokenizer : std::ctype<char> {
  new_line_tokenizer() : std::ctype<char>(get_table()) {}
  static mask const* get_table() {
    static mask rc[table_size];
    rc['\n'] = std::ctype_base::space;
    return &rc[0];
  }
};

int main() {
  std::vector<std::string> stations;

  try {
    std::fstream in("stations.txt");
    in.exceptions(std::ifstream::failbit);
    in.imbue(std::locale(std::cin.getloc(), new new_line_tokenizer));
    std::copy(std::istream_iterator<std::string>(in),
              std::istream_iterator<std::string>(),
              std::back_inserter(stations));
  } catch (std::exception const& e) {
    std::cout << "unable to read file stations.txt\n";
    return 1;
  }

  guesser g(stations);

  std::string input;
  while (std::cout << "$ " && std::getline(std::cin, input)) {
    using std::chrono::system_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    auto start = system_clock::now();
    auto candidates = g.guess(input);
    auto duration = duration_cast<milliseconds>(system_clock::now() - start);
    std::cout << "\n\t" << duration.count() << "ms\n\n";

    for (auto const& guess : candidates) {
      std::cout << "  " << stations[guess] << "\n";
    }
    std::cout << "\n";
  }
}