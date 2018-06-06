#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include "common.h"

namespace address_typeahead {

template <class Archive>
void serialize(Archive& archive, coordinates& c) {
  archive(c.lon_, c.lat_);
}

template <class Archive>
void serialize(Archive& archive, area& a) {
  archive(a.name_idx_, a.level_, a.popularity_);
}

template <class Archive>
void serialize(Archive& archive, location& l) {
  archive(l.name_idx_, l.coordinates_, l.areas_);
}

template <class Archive>
void serialize(Archive& archive, house_number& hn) {
  archive(hn.hn_idx_, hn.coordinates_);
}

template <class Archive>
void serialize(Archive& archive, street& s) {
  archive(s.name_idx_, s.house_numbers_, s.areas_);
}

template <class Archive>
void serialize(Archive& archive, typeahead_context& tc) {
  archive(tc.places_, tc.streets_, tc.areas_, tc.names_, tc.area_names_,
          tc.house_numbers_);
}

}  // namespace address_typeahead
