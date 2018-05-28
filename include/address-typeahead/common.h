#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/multi_polygon.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/ring.hpp>

namespace address_typeahead {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<int32_t, 2, bg::cs::cartesian> point;
typedef bg::model::box<point> box;
typedef bg::model::ring<point, false, true> ring;
typedef bg::model::polygon<point, false, true> polygon;
typedef bg::model::multi_polygon<polygon> multi_polygon;
typedef std::pair<box, uint64_t> value;

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
uint32_t const POSTCODE(1 << 13);

struct coordinates {
  int32_t lon_;
  int32_t lat_;
};

struct area {
  std::string name_;
  uint32_t level_;
  float popularity_;

  bool operator<(area const& other) const { return level_ < other.level_; }
};

struct location {
  coordinates coordinates_;
  std::string name_;
  std::vector<uint64_t> areas_;

  bool operator<(location const& other) const { return name_ < other.name_; }
};

struct house_number {
  std::string name_;
  coordinates coordinates_;
};

struct street {
  std::string name_;
  std::vector<house_number> house_numbers_;
  std::vector<uint64_t> areas_;
};

struct typeahead_context {

  std::vector<location> places_;
  std::vector<street> streets_;
  std::vector<area> areas_;

  bool get_coordinates(size_t id, double& lat, double& lon) const;

  bool coordinates_for_house_number(size_t id, std::string house_number,
                                    double& lat, double& lon) const;

  std::vector<size_t> get_area_ids(size_t id,
                                   uint32_t const levels = 0xffffffff) const;

  std::string get_name(size_t id) const;
  std::vector<std::string> get_all_names() const;
  std::vector<std::pair<std::string, float>> get_all_names_weighted() const;
  std::vector<std::string> get_area_names(
      size_t id, uint32_t const levels = 0xffffffff) const;
  std::vector<std::pair<std::string, float>> get_area_names_weighted(
      size_t id, uint32_t const levels = 0xffffffff) const;
  std::vector<std::string> get_area_names_sorted(
      size_t id, uint32_t const levels = 0xffffffff) const;
  std::vector<std::string> get_house_numbers(size_t id) const;

  bool is_place(size_t id) const;
  bool is_street(size_t id) const;
};

}  // namespace address_typeahead
