#include <unordered_map>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/multi_polygon.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>
#include <osmium/dynamic_handler.hpp>
#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <osmium/io/pbf_input.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm.hpp>
#include <osmium/visitor.hpp>

#include "address-typeahead/common.h"

using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type,
                                               osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

namespace address_typeahead {

class geometry_handler : public osmium::handler::Handler {
public:
  explicit geometry_handler(std::vector<address_typeahead::area>& areas)
      : areas_(areas), population_sum_(0) {}

  void area(osmium::Area const& n) {
    auto const name_tag = n.tags()["name"];
    auto const postal_code_tag = n.tags()["postal_code"];
    if (!name_tag && !postal_code_tag) {
      return;
    }

    address_typeahead::area a;
    auto const population_tag = n.tags()["population"];
    if (population_tag) {
      auto const population = atol(population_tag);
      a.popularity_ = static_cast<float>(population);
      population_sum_ += population;
    } else {
      a.popularity_ = 0;
    }

    if (name_tag) {
      auto const admin_level_tag = n.tags()["admin_level"];
      if (!admin_level_tag) {
        return;
      }

      a.name_ = name_tag;
      auto const admin_level = atol(admin_level_tag);
      if (admin_level > ADMIN_LEVEL_MAX) {
        return;
      }

      a.level_ = 1 << admin_level;
    } else {
      a.name_ = postal_code_tag;
      a.level_ = POSTCODE;
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

  std::vector<address_typeahead::area>& areas_;
  std::vector<multi_polygon> polygons_;
  uint64_t population_sum_;
};

class place_extractor : public osmium::handler::Handler {
public:
  explicit place_extractor(osmium::TagsFilter const& filter)
      : filter_(filter) {}

  void way(osmium::Way const& w) {
    if (!w.tags()["name"] || w.nodes().empty()) {
      return;
    }

    for (auto const& tag : w.tags()) {
      if (!filter_(tag)) {
        return;
      }
    }

    auto const name = std::string(w.tags()["name"]);
    if (name.length() >= 3) {
      location loc;
      loc.coordinates_ =
          point(w.nodes()[0].location().x(), w.nodes()[0].location().y());
      loc.name_ = "";

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
    for (auto const& tag : n.tags()) {
      if (!filter_(tag)) {
        return;
      }
    }

    location loc;
    loc.coordinates_ = point(n.location().x(), n.location().y());

    if (n.tags()["addr:housenumber"] && n.tags()["addr:street"]) {

      loc.name_ = std::string(n.tags()["addr:housenumber"]);

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

    if (n.tags()["name"]) {
      auto const name = std::string(n.tags()["name"]);

      if (name.length() >= 3) {
        loc.name_ = "";
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

  osmium::TagsFilter const& filter_;
  std::unordered_map<std::string, std::vector<location>> streets_;
};

std::vector<uint64_t> get_area_ids(
    point const& p, bgi::rtree<value, bgi::linear<16>> const& rtree,
    std::vector<multi_polygon> const& polygons, bool exact) {
  auto results = std::vector<uint64_t>();
  if (rtree.empty()) {
    return results;
  }

  auto query_list = std::vector<value>();
  auto const query_box = box(p, p);
  rtree.query(bgi::intersects(query_box), std::back_inserter(query_list));

  if (exact) {
    for (auto const& query_result : query_list) {
      if (bg::within(p, polygons[query_result.second])) {
        results.emplace_back(query_result.second);
      }
    }
  } else {
    for (auto const& query_result : query_list) {
      results.emplace_back(query_result.second);
    }
  }

  if (results.empty()) {
    rtree.query(bgi::nearest(p, 1), std::back_inserter(query_list));
    results.emplace_back(query_list[0].second);
  }
  return results;
}

void compress_streets(
    typeahead_context& context,
    std::unordered_map<std::string, std::vector<location>>& streets) {
  for (auto& str_it : streets) {
    if (str_it.second.size() == 1) {
      if (str_it.second[0].name_ == "") {
        location loc;
        loc.name_ = str_it.first;
        loc.coordinates_ = str_it.second[0].coordinates_;
        loc.areas_ = str_it.second[0].areas_;
        context.places_.emplace_back(loc);
      } else {
        street str;
        str.name_ = str_it.first;
        str.areas_ = str_it.second[0].areas_;
        for (auto const& loc : str_it.second) {
          house_number hn;
          hn.name_ = loc.name_;
          hn.coordinates_ = loc.coordinates_;
          str.house_numbers_.emplace_back(hn);
        }
        context.streets_.emplace_back(str);
      }
    } else {
      std::sort(str_it.second.begin(), str_it.second.end());

      auto num_of_places = size_t(0);
      for (auto const& loc : str_it.second) {
        if (loc.name_ != "") {
          break;
        }
        ++num_of_places;
      }

      auto places = std::vector<location>();
      for (size_t i = 0; i != num_of_places; ++i) {
        auto const& loc_i = str_it.second[i];

        bool found = false;
        for (size_t j = i + 1; j < num_of_places; ++j) {
          auto const& loc_j = str_it.second[j];
          if (loc_j.areas_ == loc_i.areas_) {
            found = true;
            break;
          }
        }

        if (!found) {
          location loc;
          loc.name_ = str_it.first;
          loc.coordinates_ = loc_i.coordinates_;
          loc.areas_ = loc_i.areas_;
          places.emplace_back(loc);
        }
      }

      if (num_of_places == str_it.second.size()) {
        context.places_.insert(context.places_.end(), places.begin(),
                               places.end());
      } else {

        auto house_numbers = std::vector<location>();
        for (size_t i = num_of_places; i != str_it.second.size(); ++i) {
          house_numbers.emplace_back(str_it.second[i]);
        }

        for (auto const& pl : places) {
          bool found = false;
          for (auto const& hn : house_numbers) {
            if (hn.areas_ == pl.areas_) {
              found = true;
              break;
            }
          }

          if (!found) {
            context.places_.emplace_back(pl);
          }
        }

        std::vector<street> unique_streets;
        for (auto const& loc : house_numbers) {
          bool found = false;
          for (auto& ustr : unique_streets) {
            if (ustr.areas_ == loc.areas_) {
              for (auto const& hn : ustr.house_numbers_) {
                if (hn.name_ == loc.name_) {
                  found = true;
                  break;
                }
              }

              if (!found) {
                house_number hn;
                hn.name_ = loc.name_;
                hn.coordinates_ = loc.coordinates_;
                ustr.house_numbers_.emplace_back(hn);
              }

              found = true;
              break;
            }
          }
          if (!found) {
            street new_street;
            new_street.name_ = str_it.first;
            new_street.areas_ = loc.areas_;
            house_number hn;
            hn.name_ = loc.name_;
            hn.coordinates_ = loc.coordinates_;
            new_street.house_numbers_.emplace_back(hn);
            unique_streets.emplace_back(new_street);
          }
        }

        context.streets_.insert(context.streets_.end(), unique_streets.begin(),
                                unique_streets.end());
      }
    }
  }
}

typeahead_context extract(std::string const& input_path,
                          osmium::TagsFilter const& filter, bool exact) {
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
  osmium::relations::read_relations(input_file, mp_manager);

  index_type index;
  location_handler_type location_handler(index);
  location_handler.ignore_errors();

  // second pass : read all objects & run them first through the node location
  // handler and then the multipolygon collector
  typeahead_context context;
  osmium::io::Reader reader(input_file);
  auto geom_handler = geometry_handler(context.areas_);
  osmium::apply(
      reader, location_handler,
      mp_manager.handler([&geom_handler](osmium::memory::Buffer&& buffer) {
        osmium::apply(buffer, geom_handler);
      }));
  reader.close();

  auto const inv_population_sum =
      1.0 / static_cast<double>(geom_handler.population_sum_);
  for (auto& area : context.areas_) {
    area.popularity_ = 1.0 + area.popularity_ * inv_population_sum;
  }

  bgi::rtree<value, bgi::linear<16>> rtree;
  for (size_t i = 0; i != geom_handler.polygons_.size(); ++i) {
    box b = bg::return_envelope<box>(geom_handler.polygons_[i]);
    rtree.insert(std::make_pair(b, i));
  }

  osmium::io::Reader reader2(
      input_file, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way,
      osmium::io::read_meta::no);
  auto place_handler = place_extractor(filter);
  osmium::apply(reader2, location_handler, place_handler);
  reader2.close();

  for (auto& str_it : place_handler.streets_) {
    for (auto& loc : str_it.second) {
      loc.areas_ =
          get_area_ids(loc.coordinates_, rtree, geom_handler.polygons_, exact);
    }
  }
  compress_streets(context, place_handler.streets_);

  return context;
}

}  // namespace address_typeahead
