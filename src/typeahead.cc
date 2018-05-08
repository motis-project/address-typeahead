#include "address-typeahead/typeahead.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>

#include "guess/guesser.h"

using namespace guess;

namespace address_typeahead {

std::vector<std::string> lines(std::istream& in) {
  std::vector<std::string> l;
  std::string line;
  while (!in.eof() && in.peek() != EOF) {
    std::getline(in, line);
    l.push_back(line);
  }
  return l;
}

typeahead::typeahead(std::istream& in) : names_(lines(in)), guesser_(names_) {}

typeahead::typeahead(std::vector<std::string> const strings)
    : names_(strings), guesser_(names_) {}

std::vector<std::string> typeahead::complete(std::string const& user_input) {
  std::vector<std::string> guesses;
  for (auto const& index : guesser_.guess(user_input, 10)) {
    guesses.push_back(names_[index]);
  }
  return guesses;
}

}  // namespace address_typeahead
