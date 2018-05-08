#include <algorithm>
#include <fstream>
#include <ostream>

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

#include "address-typeahead/common.h"

using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type,
                                               osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

namespace address_typeahead {

class geometry_handler : public osmium::handler::Handler {
public:
  explicit geometry_handler(std::map<uint32_t, address_typeahead::area>& areas)
      : areas_(areas) {}

  void area(osmium::Area const& n) {
    auto const name_tag = n.tags()["name"];
    auto const postal_code_tag = n.tags()["postal_code"];
    if (!name_tag && !postal_code_tag) {
      return;
    }

    std::string name;
    uint32_t level;
    if (name_tag) {
      auto const admin_level_tag = n.tags()["admin_level"];
      if (!admin_level_tag) {
        return;
      }

      name = name_tag;
      auto const admin_level = atol(admin_level_tag);
      assert(admin_level <= ADMIN_LEVEL_MAX);

      level = 1 << admin_level;
    } else {
      name = postal_code_tag;
      level = POSTCODE;
    }

    auto area_it = areas_.find(hash_string(name));
    if (area_it == areas_.end()) {
      address_typeahead::area a;
      a.name_ = name;
      a.level_ = level;
      area_it = areas_.emplace(hash_string(name), a).first;
    }

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

      area_it->second.mpolygon_.push_back(pol);
    }
  }

  std::map<uint32_t, address_typeahead::area>& areas_;
};

class place_extractor : public osmium::handler::Handler {
public:
  explicit place_extractor(std::map<uint32_t, place>& places)
      : places_(places) {}

  void way(osmium::Way const& w) {
    auto const name = w.tags()["name"];
    if (!name || w.nodes().empty()) {
      return;
    }

    auto const& n = w.nodes()[0];
    auto const coords = point(n.location().x(), n.location().y());
    auto const addr = address(coords, nullptr);

    place p;
    p.name_ = name;

    auto const place_it = places_.find(hash_string(p.name_));
    if (place_it == places_.end()) {
      p.addresses_.emplace_back(addr);
      places_.emplace(hash_string(p.name_), p);
    } else {
      place_it->second.addresses_.emplace_back(addr);
    }
  }

  void node(osmium::Node const& n) {
    auto const name = n.tags()["name"];
    auto const addr_street = n.tags()["addr:street"];

    if (!name && !addr_street) {
      return;
    }

    place p;
    p.name_ = (name != nullptr) ? name : addr_street;

    auto const coords = point(n.location().x(), n.location().y());
    auto const addr = address(coords, n.tags()["addr:housenumber"]);

    auto const place_it = places_.find(hash_string(p.name_));
    if (place_it == places_.end()) {
      p.addresses_.emplace_back(addr);
      places_.emplace(hash_string(p.name_), p);
    } else {
      place_it->second.addresses_.emplace_back(addr);
    }
  }

  std::map<uint32_t, place>& places_;
};

void extract(std::string const& input_path, typeahead_context& context) {

  auto handler = geometry_handler(context.areas_);

  osmium::io::File input_file(input_path);

  osmium::area::Assembler::config_type assembler_config;
  assembler_config.create_empty_areas = false;

  osmium::TagsFilter filter(false);
  filter.add_rule(true, "boundary");
  filter.add_rule(true, "type", "boundary");
  filter.add_rule(true, "type", "multipolygon");

  osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager(
      assembler_config, filter);

  // first pass : read relations
  osmium::relations::read_relations(input_file, mp_manager);

  index_type index;
  location_handler_type location_handler(index);
  location_handler.ignore_errors();

  // second pass : read all objects & run them first through the node location
  // handler and then the multipolygon collector
  osmium::io::Reader reader(input_file);
  osmium::apply(reader, location_handler,
                mp_manager.handler([&handler](osmium::memory::Buffer&& buffer) {
                  osmium::apply(buffer, handler);
                }));
  reader.close();

  osmium::io::Reader reader2(
      input_file, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way);
  auto handler2 = place_extractor(context.places_);
  osmium::apply(reader2, location_handler, handler2);
  reader2.close();

  for (auto const& area_it : context.areas_) {
    box b = bg::return_envelope<box>(area_it.second.mpolygon_);
    context.rtree_.insert(std::make_pair(b, area_it.first));
  }
}

}  // namespace address_typeahead
