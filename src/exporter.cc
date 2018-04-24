#include "address-typeahead/exporter.h"

#include <cassert>
#include <cstring>

namespace address_typeahead {

void write_string(std::ostream& out, std::string const& s) {
  auto const* str = s.c_str();
  auto const str_len = strlen(str);
  assert(str_len < 255);
  uint8_t len = static_cast<uint8_t>(str_len);

  out.write((char* const) & len, sizeof(len));
  out.write(str, len);
}

void write_aabb(std::ostream& out, aabb const& aabb) {
  out.write((char* const) & aabb.min.x, sizeof(int32_t));
  out.write((char* const) & aabb.min.y, sizeof(int32_t));
  out.write((char* const) & aabb.max.x, sizeof(int32_t));
  out.write((char* const) & aabb.max.y, sizeof(int32_t));
}

void write_u32(std::ostream& out, uint32_t const value) {
  out.write((char* const) & value, sizeof(value));
}

void write_point(std::ostream& out, point const& p) {
  out.write((char* const) & p.x, sizeof(p.x));
  out.write((char* const) & p.y, sizeof(p.y));
}

void binary_exporter::export_data(std::ostream& out,
                                  typeahead_context const& data) {
  write_u32(out, data.place_names_.size());
  write_u32(out, data.areas_.size());
  write_u32(out, data.places_.size());

  for (auto const& place_name : data.place_names_) {
    write_string(out, place_name.second);
  }

  for (auto const& place : data.places_) {
    write_u32(out, place.name_id_);
    write_point(out, place.coordinates_);
  };

  for (auto const& area : data.areas_) {
    auto const& area_ref = area.second;
    write_string(out, area_ref.name_);

    write_aabb(out, area_ref.bounds_);
    write_u32(out, area_ref.polygons_.size());

    for (auto const& polygon : area_ref.polygons_) {
      write_aabb(out, polygon.bounds_);
      write_u32(out, polygon.vertices_.size());
      write_u32(out, polygon.holes_.size());

      for (auto const& v : polygon.vertices_) {
        write_point(out, v);
      }

      for (auto const& inner_polygon : polygon.holes_) {
        write_aabb(out, inner_polygon.bounds_);
        write_u32(out, inner_polygon.vertices_.size());
        for (auto const& v : inner_polygon.vertices_) {
          write_point(out, v);
        }
      }
    }
  }
}

}  // address_typeahead
