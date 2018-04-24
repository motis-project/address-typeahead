#pragma once

namespace address_typeahead {

struct timer {

  timer(void) : start_time_(std::chrono::system_clock::now()) {}

  void elapsed_time_ms(void) {
    using std::chrono::system_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    auto const duration =
        duration_cast<milliseconds>(system_clock::now() - start_time_);
    std::cout << duration.count() << "ms\n";
  }

  void elapsed_time_s(void) {
    using std::chrono::system_clock;
    using std::chrono::duration_cast;
    using std::chrono::seconds;
    auto const duration =
        duration_cast<seconds>(system_clock::now() - start_time_);
    std::cout << duration.count() << "s\n";
  }

  std::chrono::time_point<std::chrono::system_clock> start_time_;
};

}  // namespace address-typeahead
