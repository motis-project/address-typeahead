#pragma once

#include <string>
#include <vector>

namespace guess {

struct guesser {
  struct match {
    match() = default;
    explicit match(unsigned index, float cos_sim)
        : index(index), cos_sim(cos_sim) {}
    bool operator<(match const& o) const { return cos_sim > o.cos_sim; }
    unsigned index;
    float cos_sim;
  };

  guesser(std::vector<std::pair<std::string, float>> const& candidates);

  std::vector<int> guess(std::string in, int count = 10) const;
  std::vector<match> guess_match(std::string in, int count = 10) const;

  std::vector<float> match_sqrts_;
  std::vector<std::vector<unsigned>> index_;
  std::vector<std::pair<std::string, float>> candidates_;

  std::vector<std::pair<size_t, size_t>> trigram_indices_;
  std::vector<uint16_t> trigrams_;
};

}  // namespace guess
