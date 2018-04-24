#include "address-typeahead/importer.h"

#include "address-typeahead/common.h"

namespace address_typeahead {

std::string read_string(std::istream& in) {
  static char buf[256];

  uint8_t len;
  in.read((char*)&len, sizeof(len));
  in.read(buf, len);
  buf[len] = '\0';
  return buf;
}

aabb read_aabb(std::istream& in) {
  aabb a;
  in.read((char*)&a.min.x, sizeof(int32_t));
  in.read((char*)&a.min.y, sizeof(int32_t));
  in.read((char*)&a.max.x, sizeof(int32_t));
  in.read((char*)&a.max.y, sizeof(int32_t));
  return a;
}

uint32_t read_u32(std::istream& in) {
  uint32_t value;
  in.read((char*)&value, sizeof(value));
  return value;
}

point read_point(std::istream& in) {
  point p;
  in.read((char*)&p.x, sizeof(p.x));
  in.read((char*)&p.y, sizeof(p.y));
  return p;
}

void binary_importer::import_data(std::istream& in, typeahead_context& data) {
  auto const place_names_size = read_u32(in);
  auto const areas_size = read_u32(in);
  auto const places_size = read_u32(in);

  for (size_t i = 0; i != place_names_size; ++i) {
    std::string str = read_string(in);
    data.place_names_.insert(
        std::pair<uint32_t, std::string>(hash_string(str), str));
  }

  for (size_t i = 0; i != places_size; ++i) {
    place p;
    p.name_id_ = read_u32(in);
    p.coordinates_ = read_point(in);
    data.places_.insert(p);
  }

  for (size_t i = 0; i != areas_size; ++i) {
    area a;
    a.name_ = read_string(in);
    // std::cout << i << " \'" << a.name_ << "\'\n";
    a.bounds_ = read_aabb(in);
    auto const polygons_size = read_u32(in);

    for (size_t j = 0; j != polygons_size; ++j) {
      outer_polygon pol;
      pol.bounds_ = read_aabb(in);
      auto const vertices_size = read_u32(in);
      auto const holes_size = read_u32(in);

      for (size_t v_i = 0; v_i != vertices_size; ++v_i) {
        pol.vertices_.push_back(read_point(in));
      }

      for (size_t h_i = 0; h_i != holes_size; ++h_i) {
        inner_polygon in_pol;
        in_pol.bounds_ = read_aabb(in);
        auto const in_vertices_size = read_u32(in);
        for (size_t iv_i = 0; iv_i != in_vertices_size; ++iv_i) {
          in_pol.vertices_.push_back(read_point(in));
        }
        pol.holes_.push_back(in_pol);
      }

      a.polygons_.push_back(pol);
    }
    data.areas_.insert(std::pair<uint32_t, area>(hash_string(a.name_), a));
  }
}

}  // address_typeahead
