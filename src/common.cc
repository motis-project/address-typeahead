#include "address-typeahead/common.h"

namespace address_typeahead {

bool typeahead_context::get_coordinates(index_t id, double& lat,
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

bool typeahead_context::coordinates_for_house_number(
    index_t id, std::string const& house_number, double& lat,
    double& lon) const {
  if (!is_street(id)) {
    return false;
  }

  auto const& str = streets_[id - places_.size()];
  for (auto const& hn : str.house_numbers_) {
    if (house_numbers_[hn.hn_idx_] == house_number) {
      lon = hn.coordinates_.lon_ / 10000000.0;
      lat = hn.coordinates_.lat_ / 10000000.0;
      return true;
    }
  }
  return false;
}

std::vector<index_t> typeahead_context::get_area_ids(
    index_t id, uint32_t const levels) const {
  auto result = std::vector<index_t>();
  if (is_place(id)) {
    for (auto const& area_id : places_[id].areas_) {
      auto const& a = areas_[area_id];
      if ((a.level_ & levels) != 0u) {
        result.emplace_back(area_id);
      }
    }
  } else if (is_street(id)) {
    for (auto const& area_id : streets_[id - places_.size()].areas_) {
      auto const& a = areas_[area_id];
      if ((a.level_ & levels) != 0u) {
        result.emplace_back(area_id);
      }
    }
  }
  return result;
}

std::string typeahead_context::get_name(index_t id) const {
  if (is_place(id)) {
    return names_[places_[id].name_idx_];
  } else if (is_street(id)) {
    return names_[streets_[id - places_.size()].name_idx_];
  }
  return "";
}

index_t typeahead_context::get_name_id(index_t id) const {
  if (is_place(id)) {
    return places_[id].name_idx_;
  } else if (is_street(id)) {
    return streets_[id - places_.size()].name_idx_;
  }
  return 0;
}

std::vector<std::pair<std::string, uint32_t>> typeahead_context::get_area_names(
    index_t id, uint32_t const levels) const {
  auto result = std::vector<std::pair<std::string, uint32_t>>();
  if (is_place(id) || is_street(id)) {
    auto const& area_ids = get_area_ids(id, levels);
    auto areas = std::vector<area>();
    for (auto const& area_id : area_ids) {
      areas.emplace_back(areas_[area_id]);
    }

    std::sort(areas.begin(), areas.end(),
              [](area const& a, area const& b) { return a.level_ > b.level_; });

    for (size_t i = 0; i != areas.size(); ++i) {
      auto const& area_i = areas[i];
      if (i + 1 != areas.size() && areas[i + 1].name_idx_ == area_i.name_idx_) {
        continue;
      }
      auto const admin_level = static_cast<uint32_t>(std::log2(area_i.level_));
      if (area_i.level_ != POSTCODE) {
        result.emplace_back(area_names_[area_i.name_idx_], admin_level);
      } else {
        result.emplace_back(std::to_string(area_i.name_idx_), admin_level);
      }
    }
  }
  return result;
}

std::vector<std::string> typeahead_context::get_house_numbers(
    index_t id) const {
  auto result = std::vector<std::string>();

  if (is_street(id)) {
    auto const& str = streets_[id - places_.size()];
    for (auto const& hn : str.house_numbers_) {
      result.emplace_back(house_numbers_[hn.hn_idx_]);
    }
  }
  return result;
}

bool typeahead_context::is_place(index_t id) const {
  return id < places_.size();
}

bool typeahead_context::is_street(index_t id) const {
  return (id >= places_.size() && id < places_.size() + streets_.size());
}

}  // namespace address_typeahead
