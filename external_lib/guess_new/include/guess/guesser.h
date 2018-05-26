#ifndef GUESS_GUESSER_H_
#define GUESS_GUESSER_H_

#include <string>
#include <vector>

namespace guess {

struct guesser {
  struct match {
    match() = default;
    explicit match(int index) : index_(index), cos_sim_(0) {}
    bool operator<(match const& o) const { return cos_sim_ > o.cos_sim_; }
    int index_;
    double cos_sim_;
  };

  explicit guesser(std::vector<std::string> const& candidates);
  explicit guesser(std::vector<std::pair<std::string, double>> const& candidates);

  std::vector<int> guess(std::string in, int count = 10) const;
  std::vector<match> guess_threshold(std::string in, double threshold,
                                     int min = 10) const;

private:
  void normalize_all_candidates();

  std::vector<match> match_trigrams(std::string& in) const;

  void score_exact_word_matches(std::string& in,
                                std::vector<match>& matches) const;

  static void normalize(std::string& s);

  std::vector<std::pair<std::string, double>> candidates_;
};

}  // namespace guess

#endif  // GUESS_GUESSER_H_
