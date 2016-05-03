#include "address-typeahead/typeahead.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>

#include "guess/guesser.h"

using namespace guess;

namespace address_typeahead {

std::vector<std::string> lines(std::string const& input_path) {
  std::vector<std::string> l;
  std::fstream in(input_path);
  in.exceptions(std::ifstream::failbit);
  std::string line;
  while (!in.eof() && in.peek() != EOF) {
    std::getline(in, line);
    l.push_back(line);
  }
  return l;
}

typeahead::typeahead(std::string const& input_path)
    : names_(lines(input_path)), guesser_(names_) {}

std::vector<std::string> typeahead::complete(std::string const& user_input) {
  std::vector<std::string> guesses;
  for (auto const& index : guesser_.guess(user_input, 10)) {
    guesses.push_back(names_[index]);
  }
  return guesses;
}

}  // namespace address_typeahead
