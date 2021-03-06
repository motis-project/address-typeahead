#include "address-typeahead/extractor.h"

#include <unordered_map>
#include <vector>

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/geometries/multi_polygon.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/geometry/geometries/polygon.hpp"
#include "boost/geometry/geometries/ring.hpp"

#include "osmium/area/assembler.hpp"
#include "osmium/area/multipolygon_manager.hpp"
#include "osmium/dynamic_handler.hpp"
#include "osmium/handler.hpp"
#include "osmium/handler/node_locations_for_ways.hpp"
#include "osmium/index/map/flex_mem.hpp"
#include "osmium/io/pbf_input.hpp"
#include "osmium/io/xml_input.hpp"
#include "osmium/memory/buffer.hpp"
#include "osmium/osm.hpp"
#include "osmium/visitor.hpp"

#include "utl/progress_tracker.h"

#include "address-typeahead/common.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

using point = bg::model::point<int32_t, 2, bg::cs::cartesian>;
using box = bg::model::box<point>;
using ring = bg::model::ring<point, false, true>;
using polygon = bg::model::polygon<point, false, true>;
using multi_polygon = bg::model::multi_polygon<polygon>;
using value = std::pair<box, address_typeahead::index_t>;

using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type,
                                               osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

namespace address_typeahead {

class geometry_handler : public osmium::handler::Handler {
public:
  explicit geometry_handler(std::vector<address_typeahead::area>& areas)
      : areas_(areas), population_sum_(0), index_(0) {}

  void area(osmium::Area const& n) {
    auto const name_tag = n.tags()["name"];
    auto const postal_code_tag = n.tags()["postal_code"];
    if ((name_tag == nullptr) && (postal_code_tag == nullptr)) {
      return;
    }

    address_typeahead::area a{};
    auto const population_tag = n.tags()["population"];
    if (population_tag != nullptr) {
      auto const population = atol(population_tag);
      a.popularity_ = static_cast<float>(population);
      population_sum_ += population;
    } else {
      a.popularity_ = 0;
    }

    if (name_tag != nullptr) {
      auto const admin_level_tag = n.tags()["admin_level"];
      if (admin_level_tag == nullptr) {
        return;
      }

      auto name_it = names_.find(name_tag);
      if (name_it == names_.end()) {
        name_it = names_.emplace(name_tag, index_++).first;
      }

      a.name_idx_ = name_it->second;
      auto const admin_level = atol(admin_level_tag);
      if (admin_level > ADMIN_LEVEL_MAX) {
        return;
      }

      a.level_ = 1 << admin_level;
    } else {
      a.name_idx_ = atol(postal_code_tag);
      a.level_ = POSTCODE;

      if (a.name_idx_ == 0) {
        return;
      }
    }
    areas_.emplace_back(a);

    multi_polygon mp;
    for (auto const& outer_ring : n.outer_rings()) {
      polygon pol;

      for (auto const& node : outer_ring) {
        pol.outer().push_back(point(node.x(), node.y()));
      }

      auto const num_inner_rings = n.inner_rings(outer_ring).size();
      if (num_inner_rings != 0) {
        pol.inners().resize(num_inner_rings);

        auto i = size_t(0);
        for (auto const& inner_ring : n.inner_rings(outer_ring)) {
          for (auto const& inner_node : inner_ring) {
            bg::append(pol.inners()[i], point(inner_node.x(), inner_node.y()));
          }
          ++i;
        }
      }

      mp.push_back(pol);
    }
    polygons_.push_back(mp);
  }

  std::unordered_map<std::string, index_t> names_;
  std::vector<address_typeahead::area>& areas_;
  std::vector<multi_polygon> polygons_;

  uint64_t population_sum_;
  index_t index_;
};

class place_extractor : public osmium::handler::Handler {
public:
  explicit place_extractor(osmium::TagsFilter const& whitelist,
                           osmium::TagsFilter const& blacklist)
      : whitelist_(whitelist), blacklist_(blacklist), hn_index_(0) {
    house_numbers_.emplace("", hn_index_++);
  }

  void way(osmium::Way const& w) {
    if ((w.tags()["name"] == nullptr) || w.nodes().empty()) {
      return;
    }

    auto found_in_whitelist = false;
    for (auto const& tag : w.tags()) {
      if (whitelist_(tag)) {
        found_in_whitelist = true;
        break;
      }
    }
    if (!found_in_whitelist) {
      return;
    }

    for (auto const& tag : w.tags()) {
      if (blacklist_(tag)) {
        return;
      }
    }

    auto const name = std::string(w.tags()["name"]);
    if (name.length() >= 3) {
      location loc;
      loc.coordinates_ = {w.nodes()[0].location().x(),
                          w.nodes()[0].location().y()};
      loc.name_idx_ = 0;

      auto const& street_it = streets_.find(name);
      if (street_it == streets_.end()) {
        std::vector<location> locs;
        locs.emplace_back(loc);
        streets_.emplace(name, locs);
      } else {
        street_it->second.emplace_back(loc);
      }
    }
  }

  void node(osmium::Node const& n) {
    auto found_in_whitelist = false;
    for (auto const& tag : n.tags()) {
      if (whitelist_(tag)) {
        found_in_whitelist = true;
        break;
      }
    }
    if (!found_in_whitelist) {
      return;
    }

    for (auto const& tag : n.tags()) {
      if (blacklist_(tag)) {
        return;
      }
    }

    location loc;
    loc.coordinates_ = {n.location().x(), n.location().y()};

    if ((n.tags()["addr:housenumber"] != nullptr) &&
        (n.tags()["addr:street"] != nullptr)) {

      auto house_number = std::string(n.tags()["addr:housenumber"]);
      auto hn_it = house_numbers_.find(house_number);
      if (hn_it == house_numbers_.end()) {
        hn_it = house_numbers_.emplace(house_number, hn_index_++).first;
      }
      loc.name_idx_ = hn_it->second;

      auto const street_name = std::string(n.tags()["addr:street"]);

      if (street_name.length() >= 3) {
        auto const& street_it = streets_.find(street_name);
        if (street_it == streets_.end()) {
          std::vector<location> locs;
          locs.emplace_back(loc);
          streets_.emplace(street_name, locs);
        } else {
          street_it->second.emplace_back(loc);
        }
      }
    }

    if (n.tags()["name"] != nullptr) {
      auto const name = std::string(n.tags()["name"]);

      if (name.length() >= 3) {
        loc.name_idx_ = 0;
        auto const& street_it = streets_.find(name);
        if (street_it == streets_.end()) {
          std::vector<location> locs;
          locs.emplace_back(loc);
          streets_.emplace(name, locs);
        } else {
          street_it->second.emplace_back(loc);
        }
      }
    }
  }

  osmium::TagsFilter const& whitelist_;
  osmium::TagsFilter const& blacklist_;
  std::unordered_map<std::string, std::vector<location>> streets_;
  std::unordered_map<std::string, index_t> house_numbers_;
  index_t hn_index_;
};

std::vector<index_t> get_area_ids(
    point const& p, bgi::rtree<value, bgi::linear<16>> const& rtree,
    std::vector<multi_polygon> const& polygons) {
  auto results = std::vector<index_t>();
  if (rtree.empty()) {
    return results;
  }

  auto query_list = std::vector<value>();
  rtree.query(bgi::contains(p), std::back_inserter(query_list));

  for (auto const& query_result : query_list) {
    if (bg::within(p, polygons[query_result.second])) {
      results.emplace_back(query_result.second);
    }
  }

  if (results.empty()) {
    rtree.query(bgi::nearest(p, 1), std::back_inserter(query_list));
    results.emplace_back(query_list[0].second);
  }
  return results;
}

void remove_duplicates(typeahead_context& context,
                       place_extractor& place_handler) {
  for (auto& place_entry : place_handler.streets_) {
    auto const name_idx = context.names_.size();
    context.names_.emplace_back(place_entry.first);

    std::sort(place_entry.second.begin(), place_entry.second.end());

    std::map<std::vector<index_t>, std::vector<std::pair<index_t, coordinates>>>
        places;
    for (auto& loc : place_entry.second) {
      std::sort(loc.areas_.begin(), loc.areas_.end());
      auto it = places.find(loc.areas_);
      if (it == places.end()) {
        it = places
                 .emplace(loc.areas_,
                          std::vector<std::pair<index_t, coordinates>>())
                 .first;
      }
      it->second.emplace_back(loc.name_idx_, loc.coordinates_);
    }

    for (auto const& unique_place : places) {
      std::vector<house_number> house_numbers;
      for (size_t i = 0; i != unique_place.second.size(); ++i) {
        auto const& loc = unique_place.second[i];
        if (loc.first == 0 && (i + 1 == unique_place.second.size())) {
          location new_place;
          new_place.name_idx_ = name_idx;
          new_place.coordinates_ = loc.second;
          new_place.areas_ = unique_place.first;
          context.places_.emplace_back(new_place);
        } else if (loc.first != 0) {
          house_numbers.emplace_back(house_number{loc.first, loc.second});
        }
      }
      if (!house_numbers.empty()) {
        street new_street;
        new_street.name_idx_ = name_idx;
        new_street.house_numbers_ = house_numbers;
        new_street.areas_ = unique_place.first;
        context.streets_.emplace_back(new_street);
      }
    }
  }
}

void split_box(box const& b, int32_t const max_dim, std::vector<box>& boxes,
               multi_polygon const& polygon) {
  auto const min_x = b.min_corner().get<0>();
  auto const max_x = b.max_corner().get<0>();
  auto const min_y = b.min_corner().get<1>();
  auto const max_y = b.max_corner().get<1>();

  auto const width = max_x - min_x;
  auto const height = max_y - min_y;
  auto const half_width = width >> 1;
  auto const half_height = height >> 1;

  if (width <= max_dim && height <= max_dim) {
    if (bg::covered_by(point(min_x + half_width, min_y + half_height),
                       polygon)) {
      boxes.emplace_back(b);
    }
    return;
  }

  if (width > max_dim) {
    auto const half_x = min_x + half_width;
    box left_box(point(min_x, min_y), point(half_x, max_y));
    split_box(left_box, max_dim, boxes, polygon);

    box right_box(point(half_x + 1, min_y), point(max_x, max_y));
    split_box(right_box, max_dim, boxes, polygon);
  } else {
    auto const half_y = min_y + half_height;
    box top_box(point(min_x, half_y + 1), point(max_x, max_y));
    split_box(top_box, max_dim, boxes, polygon);

    box bottom_box(point(min_x, min_y), point(max_x, half_y));
    split_box(bottom_box, max_dim, boxes, polygon);
  }
}

void extract_options::whitelist_add(std::string const& tag,
                                    std::string const& value) {
  if (value.empty()) {
    whitelist_.add_rule(true, tag);
  } else {
    whitelist_.add_rule(true, tag, value);
  }
}

void extract_options::blacklist_add(std::string const& tag,
                                    std::string const& value) {
  if (value.empty()) {
    blacklist_.add_rule(true, tag);
  } else {
    blacklist_.add_rule(true, tag, value);
  }
}

typeahead_context extract(std::string const& input_path,
                          extract_options const& options) {
  auto progress_tracker =
      utl::get_active_progress_tracker_or_activate("address");
  progress_tracker->show_progress(true);

  osmium::io::File input_file(input_path);

  osmium::area::Assembler::config_type assembler_config;
  assembler_config.create_empty_areas = false;

  osmium::TagsFilter mp_filter(false);
  mp_filter.add_rule(true, "boundary");
  mp_filter.add_rule(true, "type", "boundary");
  mp_filter.add_rule(true, "type", "multipolygon");

  osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager(
      assembler_config, mp_filter);

  // first pass : read relations
  progress_tracker->status("1st Pass / Relations").out_bounds(0.F, 25.F);
  osmium::relations::read_relations(input_file, mp_manager);

  index_type index;
  location_handler_type location_handler(index);
  location_handler.ignore_errors();

  // second pass : read all objects & run them first through the node location
  // handler and then the multipolygon collector and the place extractor
  osmium::io::Reader reader(
      input_file, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way,
      osmium::io::read_meta::no);
  progress_tracker->status("2nd Pass / Geometry")
      .out_bounds(25.F, 50.F)
      .in_high(reader.file_size());
  typeahead_context context;
  auto geom_handler = geometry_handler(context.areas_);
  auto place_handler = place_extractor(options.whitelist_, options.blacklist_);
  osmium::apply(
      reader, [&](auto&&) { progress_tracker->update(reader.offset()); },
      location_handler,
      mp_manager.handler([&geom_handler](osmium::memory::Buffer&& buffer) {
        osmium::apply(buffer, geom_handler);
      }),
      place_handler);
  reader.close();

  auto const inv_population_sum =
      1.0 / static_cast<double>(geom_handler.population_sum_);
  for (auto& area : context.areas_) {
    area.popularity_ = 1.0 + area.popularity_ * inv_population_sum;
  }

  std::vector<value> values;
  for (index_t i = 0; i != geom_handler.polygons_.size(); ++i) {
    auto b = bg::return_envelope<box>(geom_handler.polygons_[i]);
    assert(b.min_corner().get<0>() <= b.max_corner().get<0>());
    assert(b.min_corner().get<1>() <= b.max_corner().get<1>());
    values.emplace_back(b, i);
  }

  std::vector<value> final_values;
  if (options.approximation_lvl_ < APPROX_LVL_1 ||
      options.approximation_lvl_ > APPROX_LVL_5) {
    final_values.insert(final_values.end(), values.begin(), values.end());
  } else {
    progress_tracker->status("Approximate Polygons")
        .out_bounds(50.F, 75.F)
        .in_high(values.size());

    auto const max_dim = options.approximation_lvl_;
    for (index_t i = 0; i != values.size(); ++i) {
      progress_tracker->increment();
      std::vector<box> split_boxes;
      split_box(values[i].first, max_dim, split_boxes,
                geom_handler.polygons_[i]);
      for (auto const& b : split_boxes) {
        final_values.emplace_back(b, i);
      }
    }
  }

  auto rtree = bgi::rtree<value, bgi::linear<16>>();
  rtree.insert(final_values.begin(), final_values.end());

  progress_tracker->status("Generate Streets")
      .out_bounds(75.F, 100.F)
      .in_high(place_handler.streets_.size());

  for (auto& str_it : place_handler.streets_) {
    progress_tracker->increment();
    for (auto& loc : str_it.second) {
      auto const p = point(loc.coordinates_.lon_, loc.coordinates_.lat_);
      if (options.approximation_lvl_ == APPROX_NONE) {
        loc.areas_ = get_area_ids(p, rtree, geom_handler.polygons_);
      } else {
        auto query_list = std::vector<value>();
        rtree.query(bgi::covers(p), std::back_inserter(query_list));
        for (auto const& q : query_list) {
          loc.areas_.emplace_back(q.second);
        }
      }
    }
  }

  progress_tracker->status("Removing Duplicates");
  remove_duplicates(context, place_handler);

  progress_tracker->status("FINISHED").show_progress(false);

  context.area_names_.resize(geom_handler.names_.size());
  for (auto const& area_name : geom_handler.names_) {
    context.area_names_[area_name.second] = area_name.first;
  }

  context.house_numbers_.resize(place_handler.house_numbers_.size());
  for (auto const& hn : place_handler.house_numbers_) {
    context.house_numbers_[hn.second] = hn.first;
  }

  return context;
}

}  // namespace address_typeahead
