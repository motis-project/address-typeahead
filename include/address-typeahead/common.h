#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

//#include "collision.h"

namespace address_typeahead {

uint32_t hash_string(std::string const& s);

struct point {
  int32_t x, y;
};

struct aabb {
  point min, max;

  bool contains(point const& p) const {
    if (p.x < min.x || p.x > max.x) return false;
    if (p.y < min.y || p.y > max.y) return false;
    return true;
  }
};

bool point_in_poly(std::vector<point> const& vertices, point const& p);

struct inner_polygon {
  std::vector<point> vertices_;
  aabb bounds_;

  bool contains(point const& p) const {
    if (!bounds_.contains(p)) return false;
    return point_in_poly(vertices_, p);
  }
};

struct outer_polygon {
  std::vector<point> vertices_;

  std::vector<inner_polygon> holes_;

  aabb bounds_;

  bool contains(point const& p) const {
    if (!bounds_.contains(p)) return false;
    if (!point_in_poly(vertices_, p)) return false;

    for (auto const& hole : holes_) {
      if (hole.contains(p)) return false;
    }

    return true;
  }
};

struct area {
  std::string name_;

  aabb bounds_;

  std::vector<outer_polygon> polygons_;

  bool contains(point const& p) const {
    if (!bounds_.contains(p)) return false;

    for (auto const& pol : polygons_) {
      if (pol.contains(p)) return true;
    }
    return false;
  }
};

struct place {
  uint32_t name_id_;
  point coordinates_;

  bool operator<(place const& other) const {
    return (name_id_ < other.name_id_);
  }
};

struct typeahead_context {
  std::map<uint32_t, std::string> place_names_;
  std::map<uint32_t, area> areas_;
  std::set<place> places_;
};

std::vector<uint32_t> get_areas(point const& p,
                                std::map<uint32_t, area> const& areas);

}  // address_typeahead
