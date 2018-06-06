#include "guess/guesser.h"

#include <cinttypes>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "guess/string_util.h"
#include "guess/trigram_util.h"

namespace guess {

unsigned trigram_match_count(std::string const& in, std::string const& candidate) {
  auto matches = 0u;
  for_each_trigram(in, [&](auto&& in_trigram) {
    auto cont = true;
    for_each_trigram(candidate, [&](auto&& candidate_trigram) {
      if (cont && in_trigram == candidate_trigram) {
        ++matches;
        cont = false;
      }
    });
  });
  return matches;
}

guesser::guesser(
    std::vector<std::pair<std::string, float>> const& candidates) {
  index_.resize(max_compressed_trigram + 1);
  match_sqrts_.resize(candidates.size());
  candidates_.resize(candidates.size());

  for (auto i = 0u; i < candidates.size(); ++i) {
    candidates_[i] = candidates[i];

    auto& str = candidates_[i].first;
    normalize(str);
    match_sqrts_[i] = static_cast<float>(std::sqrt(str.size() - 2));
    for_each_trigram(str, [&](auto&& t) { index_[t].push_back(i); });
    str.shrink_to_fit();
  }

  match_sqrts_.shrink_to_fit();
  candidates_.shrink_to_fit();
  index_.shrink_to_fit();
  for (auto& match_indices : index_) {
    std::sort(begin(match_indices), end(match_indices));
    match_indices.erase(std::unique(begin(match_indices), end(match_indices)),
                        end(match_indices));
    match_indices.shrink_to_fit();
  }
}

std::vector<int> guesser::guess(std::string in, int count) const {
  normalize(in);

  // Collect candidate indices matched by the trigrams in the input string.
  std::vector<unsigned> match_indices;
  match_indices.reserve(512);
  for_each_trigram(in, [&](auto&& trigram_input) {
    match_indices.insert(end(match_indices), begin(index_[trigram_input]),
                         end(index_[trigram_input]));
  });

  // Score the trigram matches on the candidates.
  std::vector<uint8_t> match_counts(candidates_.size());
  for (auto const& i : match_indices) {
    ++match_counts[i];
  }

  // Calculate cosine-similarity.
  std::vector<guesser::match> m;
  m.reserve(512);
  auto const sqrt_len_vec_in = static_cast<float>(std::sqrt(in.size() - 2));
  for (auto i = 1u; i < candidates_.size(); ++i) {
    if (match_counts[i] != 0) {
      auto const match_count = trigram_match_count(in, candidates_[i].first);
      m.emplace_back(i, candidates_[i].second * match_count /
                            (sqrt_len_vec_in * match_sqrts_[i]));

      // Score exact word match.
      if (m.back().cos_sim > 0.5) {
        for_each_token(candidates_[m.back().index].first, [&](char* t1) {
          for_each_token(in, [&](char* t2) {
            if (strcmp(t1, t2) == 0) {
              m.back().cos_sim *= 1.33f;
              return true;
            }
            return false;
          });
          return false;
        });
      }
    }
  }

  // Sort matches by cosine-similarity.
  auto result_count = std::min(static_cast<std::size_t>(count), m.size());
  std::nth_element(begin(m), begin(m) + result_count, end(m));
  m.resize(result_count);
  std::sort(begin(m), end(m));

  // Translate to indices.
  std::vector<int> ret(m.size());
  for (int i = 0; i < m.size(); ++i) {
    ret[i] = m[i].index;
  }

  return ret;
}

std::vector<guesser::match> guesser::guess_match(std::string in, int count) const {
  normalize(in);

  // Collect candidate indices matched by the trigrams in the input string.
  std::vector<unsigned> match_indices;
  match_indices.reserve(512);
  for_each_trigram(in, [&](auto&& trigram_input) {
    match_indices.insert(end(match_indices), begin(index_[trigram_input]),
                         end(index_[trigram_input]));
  });

  // Score the trigram matches on the candidates.
  std::vector<uint8_t> match_counts(candidates_.size());
  for (auto const& i : match_indices) {
    ++match_counts[i];
  }

  // Calculate cosine-similarity.
  std::vector<guesser::match> m;
  m.reserve(512);
  auto const sqrt_len_vec_in = static_cast<float>(std::sqrt(in.size() - 2));
  for (auto i = 1u; i < candidates_.size(); ++i) {
    if (match_counts[i] != 0) {
      auto const match_count = trigram_match_count(in, candidates_[i].first);
      m.emplace_back(i, candidates_[i].second * match_count /
                            (sqrt_len_vec_in * match_sqrts_[i]));

      // Score exact word match.
      if (m.back().cos_sim > 0.5) {
        for_each_token(candidates_[m.back().index].first, [&](char* t1) {
          for_each_token(in, [&](char* t2) {
            if (strcmp(t1, t2) == 0) {
              m.back().cos_sim *= 1.33f;
              return true;
            }
            return false;
          });
          return false;
        });
      }
    }
  }

  // Sort matches by cosine-similarity.
  auto result_count = std::min(static_cast<std::size_t>(count), m.size());
  std::nth_element(begin(m), begin(m) + result_count, end(m));
  m.resize(result_count);
  std::sort(begin(m), end(m));

  return m;
}

}  // namespace guess
