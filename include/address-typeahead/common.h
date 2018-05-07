#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/multi_polygon.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/ring.hpp>

namespace address_typeahead {

uint64_t hash_string(std::string const& s);

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<int32_t, 2, bg::cs::cartesian> point;
typedef bg::model::box<point> box;
typedef bg::model::ring<point, false, true> ring;
typedef bg::model::polygon<point, false, true> polygon;
typedef bg::model::multi_polygon<polygon> multi_polygon;
typedef std::pair<box, size_t> value;

uint32_t const ADMIN_LEVEL_0(1);
uint32_t const ADMIN_LEVEL_1(1 << 1);
uint32_t const ADMIN_LEVEL_2(1 << 2);
uint32_t const ADMIN_LEVEL_3(1 << 3);
uint32_t const ADMIN_LEVEL_4(1 << 4);
uint32_t const ADMIN_LEVEL_5(1 << 5);
uint32_t const ADMIN_LEVEL_6(1 << 6);
uint32_t const ADMIN_LEVEL_7(1 << 7);
uint32_t const ADMIN_LEVEL_8(1 << 8);
uint32_t const ADMIN_LEVEL_9(1 << 9);
uint32_t const DMIN_LEVEL_10(1 << 10);
uint32_t const ADMIN_LEVEL_11(1 << 11);
uint32_t const ADMIN_LEVEL_12(1 << 12);
uint32_t const ADMIN_LEVEL_MAX(ADMIN_LEVEL_12);
uint32_t const POSTCODE(1 << 31);

struct area {
  std::string name_;
  uint32_t level_;
  multi_polygon mpolygon_;

  bool operator<(area const& other) const { return level_ < other.level_; }
};

struct address {
  point coordinates_;
  std::string house_number_;

  address(void) {}
  address(point const& coords, char const* const house_number)
      : coordinates_(coords) {
    if (house_number == nullptr) {
      house_number_ = "";
    } else {
      house_number_ = house_number;
    }
  }
};

struct place {
  std::string name_;
  std::vector<address> addresses_;
};

struct typeahead_context {
  std::map<uint32_t, place> places_;
  std::map<uint32_t, area> areas_;
  bgi::rtree<value, bgi::linear<16>> rtree_;

  std::vector<std::string> get_all_place_names(void) const;
  place const* get_place(std::string const& place_name) const;

  std::vector<uint32_t> get_area_ids(point const& p) const;
  std::vector<uint32_t> get_area_ids(place const& pl) const;
  std::vector<uint32_t> get_area_ids_filtered(place const& pl,
                                              uint32_t const levels) const;

  std::vector<std::string> get_area_names_sorted(place const& pl) const;
  std::vector<std::string> get_housenumbers(place const& pl) const;
};

}  // address_typeahead
