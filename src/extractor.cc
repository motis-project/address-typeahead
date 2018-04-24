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

#include "address-typeahead/exporter.h"

using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type,
                                               osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

namespace address_typeahead {

class geometry_handler : public osmium::handler::Handler {
public:
  geometry_handler(std::map<uint32_t, address_typeahead::area>& areas)
      : areas_(areas) {}

  void area(osmium::Area const& n) {
    auto const name_tag = n.tags()["name"];
    if (!name_tag) return;

    std::string name = name_tag;

    auto envelope = n.envelope();
    auto bl = envelope.bottom_left();
    auto tr = envelope.top_right();

    auto area_it = areas_.find(hash_string(name));
    if (area_it == areas_.end()) {
      address_typeahead::area a;
      a.name_ = name;
      a.bounds_.min = {bl.x(), bl.y()};
      a.bounds_.max = {tr.x(), tr.y()};
      areas_.insert(
          std::pair<uint32_t, address_typeahead::area>(hash_string(name), a));
      area_it = areas_.find(hash_string(name));
    }
    address_typeahead::area& area_ref = area_it->second;

    area_ref.bounds_.min.x =
        (area_ref.bounds_.min.x < bl.x()) ? area_ref.bounds_.min.x : bl.x();
    area_ref.bounds_.min.y =
        (area_ref.bounds_.min.y < bl.y()) ? area_ref.bounds_.min.y : bl.y();
    area_ref.bounds_.max.x =
        (area_ref.bounds_.max.x < tr.x()) ? area_ref.bounds_.max.x : tr.x();
    area_ref.bounds_.max.y =
        (area_ref.bounds_.max.y < tr.y()) ? area_ref.bounds_.max.y : tr.y();

    for (auto const& outer_ring : n.outer_rings()) {
      envelope = outer_ring.envelope();
      bl = envelope.bottom_left();
      tr = envelope.top_right();
      aabb bounds;
      bounds.min.x = bl.x();
      bounds.min.y = bl.y();
      bounds.max.x = tr.x();
      bounds.max.y = tr.y();

      outer_polygon pol;
      pol.bounds_ = bounds;
      for (auto const& node : outer_ring) {
        point p;
        p.x = node.x();
        p.y = node.y();
        pol.vertices_.push_back(p);
      }

      for (auto const& inner_ring : n.inner_rings(outer_ring)) {
        envelope = outer_ring.envelope();
        bl = envelope.bottom_left();
        tr = envelope.top_right();
        bounds.min.x = bl.x();
        bounds.min.y = bl.y();
        bounds.max.x = tr.x();
        bounds.max.y = tr.y();

        inner_polygon inner_pol;
        inner_pol.bounds_ = bounds;
        for (auto const& inner_node : inner_ring) {
          point p;
          p.x = inner_node.x();
          p.y = inner_node.y();
          inner_pol.vertices_.push_back(p);
        }
        pol.holes_.push_back(inner_pol);
      }

      area_ref.polygons_.push_back(pol);
    }
  }

  std::map<uint32_t, address_typeahead::area>& areas_;
};

class street_extractor : public osmium::handler::Handler {
public:
  street_extractor(std::set<place>& places,
                   std::map<uint32_t, std::string>& names)
      : places_(places), names_(names) {}

  void node(osmium::Node const& n) {
    auto const name = n.tags()["name"];
    auto const addr_street = n.tags()["addr:street"];

    if (name) {
      place p;
      p.name_id_ = hash_string(name);
      p.coordinates_.x = n.location().x();
      p.coordinates_.y = n.location().y();
      places_.insert(p);
      names_.insert(std::pair<uint32_t, std::string>(p.name_id_, name));
    }

    if (addr_street) {
      place p;
      p.name_id_ = hash_string(addr_street);
      p.coordinates_.x = n.location().x();
      p.coordinates_.y = n.location().y();
      places_.insert(p);
      names_.insert(std::pair<uint32_t, std::string>(p.name_id_, addr_street));
    }
  }

  std::set<place>& places_;
  std::map<uint32_t, std::string>& names_;
};

void extract(std::string const& input_path, typeahead_context& context) {

  auto handler = geometry_handler(context.areas_);

  osmium::io::File input_file(input_path);

  osmium::area::Assembler::config_type assembler_config;

  osmium::TagsFilter filter(false);
  filter.add_rule(true, "boundary");

  osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager(
      assembler_config, filter);

  // first pass : read relations
  std::cout << "Pass 1... ";
  osmium::relations::read_relations(input_file, mp_manager);
  std::cout << "done\n";

  index_type index;
  location_handler_type location_handler(index);
  location_handler.ignore_errors();

  // second pass : read all objects & run them first through the node location
  // handler and then the multipolygon collector
  std::cout << "Pass 2... ";
  std::flush(std::cout);
  osmium::io::Reader reader(input_file);
  osmium::apply(reader, location_handler,
                mp_manager.handler([&handler](osmium::memory::Buffer&& buffer) {
                  osmium::apply(buffer, handler);
                }));
  std::cout << "done\n";

  osmium::io::Reader reader2(input_file);
  auto handler2 = street_extractor(context.places_, context.place_names_);
  osmium::apply(reader2, handler2);
}

}  // namespace address_typeahead
