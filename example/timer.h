#pragma once

#include <chrono>
#include <iostream>

namespace address_typeahead {

struct timer {

  timer() : start_time_(std::chrono::system_clock::now()) {}

  void elapsed_time_ms() {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::system_clock;
    auto const duration =
        duration_cast<milliseconds>(system_clock::now() - start_time_);
    std::cout << duration.count() << "ms\n";
  }

  void elapsed_time_s() {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::system_clock;
    auto const duration = duration_cast<std::chrono::duration<float>>(
        system_clock::now() - start_time_);
    std::cout << duration.count() << "s\n";
  }

  std::chrono::time_point<std::chrono::system_clock> start_time_;
};

}  // namespace address_typeahead
