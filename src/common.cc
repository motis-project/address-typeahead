#include "address-typeahead/common.h"

namespace address_typeahead {

bool typeahead_context::get_coordinates(size_t id, double& lat,
                                        double& lon) const {
  if (is_place(id)) {
    auto const& loc = places_[id];
    lon = loc.coordinates_.lon_ / 10000000.0;
    lat = loc.coordinates_.lat_ / 10000000.0;
    return true;
  } else if (is_street(id)) {
    auto const& loc = streets_[id - places_.size()].house_numbers_[0];
    lon = loc.coordinates_.lon_ / 10000000.0;
    lat = loc.coordinates_.lat_ / 10000000.0;
    return true;
  }
  return false;
}

bool typeahead_context::coordinates_for_house_number(size_t id,
                                                     std::string house_number,
                                                     double& lat,
                                                     double& lon) const {
  if (!is_street(id)) {
    return false;
  }

  auto const& str = streets_[id - places_.size()];
  for (auto const& hn : str.house_numbers_) {
    if (hn.name_ == house_number) {
      lon = hn.coordinates_.lon_ / 10000000.0;
      lat = hn.coordinates_.lat_ / 10000000.0;
      return true;
    }
  }
  return false;
}

std::vector<size_t> typeahead_context::get_area_ids(
    size_t id, uint32_t const levels) const {
  auto result = std::vector<size_t>();
  if (is_place(id)) {
    for (auto const& area_id : places_[id].areas_) {
      auto const& a = areas_[area_id];
      if (a.level_ & levels) {
        result.emplace_back(area_id);
      }
    }
  } else if (is_street(id)) {
    for (auto const& area_id : streets_[id - places_.size()].areas_) {
      auto const& a = areas_[area_id];
      if (a.level_ & levels) {
        result.emplace_back(area_id);
      }
    }
  }
  return result;
}

std::string typeahead_context::get_name(size_t id) const {
  if (is_place(id)) {
    return places_[id].name_;
  } else if (is_street(id)) {
    return streets_[id - places_.size()].name_;
  }
  return "";
}

std::vector<std::string> typeahead_context::get_all_names() const {
  auto results = std::vector<std::string>();
  results.reserve(places_.size() + streets_.size());
  for (auto const& pl : places_) {
    results.emplace_back(pl.name_);
  }
  for (auto const& str : streets_) {
    results.emplace_back(str.name_);
  }
  return results;
}

std::vector<std::pair<std::string, float>>
typeahead_context::get_all_names_weighted() const {
  auto results = std::vector<std::pair<std::string, float>>();
  results.reserve(places_.size() + streets_.size());
  for (auto const& pl : places_) {
    results.emplace_back(pl.name_, 1);
  }
  for (auto const& str : streets_) {
    results.emplace_back(str.name_, 1);
  }
  return results;
}

std::vector<std::string> typeahead_context::get_area_names(
    size_t id, uint32_t const levels) const {
  auto result = std::vector<std::string>();
  if (is_place(id) || is_street(id)) {
    auto const& area_ids = get_area_ids(id, levels);
    for (auto const& area_id : area_ids) {
      result.emplace_back(areas_[area_id].name_);
    }
  }
  return result;
}

std::vector<std::pair<std::string, float>>
typeahead_context::get_area_names_weighted(size_t id,
                                           uint32_t const levels) const {
  auto result = std::vector<std::pair<std::string, float>>();
  if (is_place(id) || is_street(id)) {
    auto const& area_ids = get_area_ids(id, levels);
    for (auto const& area_id : area_ids) {
      result.emplace_back(areas_[area_id].name_, areas_[area_id].popularity_);
    }
  }
  return result;
}

std::vector<std::string> typeahead_context::get_area_names_sorted(
    size_t id, uint32_t const levels) const {
  auto result = std::vector<std::string>();
  if (is_place(id) || is_street(id)) {
    auto const& area_ids = get_area_ids(id, levels);
    auto areas = std::vector<area>();
    for (auto const& area_id : area_ids) {
      areas.emplace_back(areas_[area_id]);
    }

    std::sort(areas.begin(), areas.end());

    for (size_t i = 0; i != areas.size(); ++i) {
      auto const& area_i = areas[i];
      if (i + 1 != areas.size() && areas[i + 1].name_ == area_i.name_) {
        continue;
      }

      result.emplace_back(area_i.name_);
    }
  }
  return result;
}

std::vector<std::string> typeahead_context::get_house_numbers(size_t id) const {
  auto result = std::vector<std::string>();

  if (is_street(id)) {
    auto const& str = streets_[id - places_.size()];
    for (auto const& hn : str.house_numbers_) {
      result.emplace_back(hn.name_);
    }
  }
  return result;
}

bool typeahead_context::is_place(size_t id) const {
  return id < places_.size();
}

bool typeahead_context::is_street(size_t id) const {
  return (id >= places_.size() && id < places_.size() + streets_.size());
}

}  // namespace address_typeahead
