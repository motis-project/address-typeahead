#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/geometries/multi_polygon.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/geometry/geometries/polygon.hpp"
#include "boost/geometry/geometries/ring.hpp"

namespace address_typeahead {

using index_t = uint32_t;

constexpr auto const ADMIN_LEVEL_0(1);
constexpr auto const ADMIN_LEVEL_1(1 << 1);
constexpr auto const ADMIN_LEVEL_2(1 << 2);
constexpr auto const ADMIN_LEVEL_3(1 << 3);
constexpr auto const ADMIN_LEVEL_4(1 << 4);
constexpr auto const ADMIN_LEVEL_5(1 << 5);
constexpr auto const ADMIN_LEVEL_6(1 << 6);
constexpr auto const ADMIN_LEVEL_7(1 << 7);
constexpr auto const ADMIN_LEVEL_8(1 << 8);
constexpr auto const ADMIN_LEVEL_9(1 << 9);
constexpr auto const DMIN_LEVEL_10(1 << 10);
constexpr auto const ADMIN_LEVEL_11(1 << 11);
constexpr auto const ADMIN_LEVEL_12(1 << 12);
constexpr auto const ADMIN_LEVEL_MAX(ADMIN_LEVEL_12);
constexpr auto const POSTCODE(1 << 13);

struct coordinates {
  int32_t lon_;
  int32_t lat_;
};

struct area {
  index_t name_idx_;
  uint32_t level_;
  float popularity_;

  bool operator<(area const& other) const { return level_ < other.level_; }
};

struct location {
  index_t name_idx_;
  coordinates coordinates_;
  std::vector<index_t> areas_;

  bool operator<(location const& other) const {
    return name_idx_ < other.name_idx_;
  }
};

struct house_number {
  index_t hn_idx_;
  coordinates coordinates_;
};

struct street {
  index_t name_idx_;
  std::vector<house_number> house_numbers_;
  std::vector<index_t> areas_;
};

struct typeahead_context {

  std::vector<location> places_;
  std::vector<street> streets_;
  std::vector<area> areas_;

  std::vector<std::string> names_;
  std::vector<std::string> area_names_;
  std::vector<std::string> house_numbers_;

  bool get_coordinates(index_t id, double& lat, double& lon) const;

  bool coordinates_for_house_number(index_t id, std::string const& house_number,
                                    double& lat, double& lon) const;

  std::vector<index_t> get_area_ids(index_t id,
                                    uint32_t const levels = 0xffffffff) const;

  std::string get_name(index_t id) const;
  index_t get_name_id(index_t id) const;

  std::vector<std::pair<std::string, uint32_t>> get_area_names(
      index_t id, uint32_t const levels = 0xffffffff) const;
  std::vector<std::string> get_house_numbers(index_t id) const;

  bool is_place(index_t id) const;
  bool is_street(index_t id) const;
};

}  // namespace address_typeahead
