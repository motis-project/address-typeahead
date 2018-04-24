#include "address-typeahead/common.h"

namespace address_typeahead {

uint32_t hash_string(std::string const& s) {
  std::hash<std::string> hash_fn;
  return (hash_fn(s));
}

bool point_in_poly(std::vector<point> const& vertices, point const& p) {
  size_t i, j;
  bool c = 0;
  for (i = 0, j = vertices.size() - 1; i < vertices.size(); j = i++) {
    if (((vertices[i].y > p.y) != (vertices[j].y > p.y)) &&
        (p.x < (vertices[j].x - vertices[i].x) * (p.y - vertices[i].y) /
                       (vertices[j].y - vertices[i].y) +
                   vertices[i].x))
      c = !c;
  }
  return c;
}

std::vector<uint32_t> get_areas(point const& p,
                                std::map<uint32_t, area> const& areas) {
  std::vector<uint32_t> result;

  for (auto const& a : areas) {
    if (a.second.contains(p)) {
      result.push_back(a.first);
    }
  }

  return result;
}

}  // namespace address_typeahead
