#include "address-typeahead/common.h"

#include <boost/functional/hash.hpp>

namespace address_typeahead {

uint64_t hash_string(std::string const& s) {
  boost::hash<std::string> hash_fn;
  return hash_fn(s);
}

void remove_duplicates(std::vector<uint32_t>& values) {
  for (size_t i = 0; i < values.size(); ++i) {
    for (size_t j = i + 1; j < values.size(); ++j) {
      if (values[i] == values[j]) {
        auto const& it = values.begin() + j;
        values.erase(it);
        --j;
      }
    }
  }
}

place const* typeahead_context::get_place(std::string const& place_name) const {
  auto const place_name_hash = hash_string(place_name);
  auto const place_it = places_.find(place_name_hash);
  if (place_it != places_.end()) {
    return &place_it->second;
  }
  return nullptr;
}

std::vector<uint32_t> typeahead_context::get_area_ids(point const& p) const {
  auto results = std::vector<uint32_t>();
  if (rtree_.size() == 0) {
    return results;
  }

  auto query_list = std::vector<value>();
  auto const query_box = box(p, p);
  rtree_.query(bgi::intersects(query_box), std::back_inserter(query_list));

  if (query_list.size() != 0) {
    for (auto const& query_result : query_list) {
      auto const& area_it = areas_.find(query_result.second);
      if (bg::within(p, area_it->second.mpolygon_)) {
        results.push_back(query_result.second);
      }
    }
  }

  if (results.size() == 0) {
    rtree_.query(bgi::nearest(p, 1), std::back_inserter(query_list));
    results.push_back(query_list[0].second);
  }

  return results;
}

std::vector<uint32_t> typeahead_context::get_area_ids(place const& pl) const {
  auto results = std::vector<uint32_t>();
  for (auto const& addr : pl.addresses_) {
    auto const& res = get_area_ids(addr.coordinates_);
    results.insert(results.end(), res.begin(), res.end());
  }
  remove_duplicates(results);

  return results;
}

std::vector<std::string> typeahead_context::get_area_names_sorted(
    place const& pl) const {
  auto const& area_ids = get_area_ids(pl);

  auto areas = std::vector<area>();
  for (auto const& id : area_ids) {
    auto const& ar = areas_.find(id);
    if (ar != areas_.end()) {
      areas.emplace_back(ar->second);
    }
  }
  std::sort(areas.begin(), areas.end());

  auto results = std::vector<std::string>();
  for (auto const& ar : areas) {
    results.emplace_back(ar.name_);
  }

  return results;
}

std::vector<uint32_t> typeahead_context::get_area_ids_filtered(
    place const& pl, uint32_t const levels) const {
  auto const& all_areas = get_area_ids(pl);

  auto results = std::vector<uint32_t>();
  for (auto const a : all_areas) {
    auto const it = areas_.find(a);
    if (it->second.level_ & levels) {
      results.push_back(a);
    }
  }

  return results;
}

std::vector<std::string> typeahead_context::get_housenumbers(
    place const& pl) const {
  auto results = std::vector<std::string>();
  for (auto const& addr : pl.addresses_) {
    results.push_back(addr.house_number_);
  }
  return results;
}

std::vector<std::string> typeahead_context::get_all_place_names(void) const {
  auto results = std::vector<std::string>();
  results.reserve(places_.size());
  for (auto const& pl : places_) {
    results.emplace_back(pl.second.name_);
  }
  return results;
}

}  // namespace address_typeahead
