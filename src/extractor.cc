#include <fstream>
#include <ostream>

#include "osmium/handler.hpp"
#include "osmium/io/pbf_input.hpp"
#include "osmium/io/xml_input.hpp"
#include "osmium/memory/buffer.hpp"
#include "osmium/osm.hpp"
#include "osmium/visitor.hpp"

namespace address_typeahead {

class street_extractor : public osmium::handler::Handler {
public:
  void way(osmium::Way const& w) {
    auto const highway = w.tags()["highway"];
    auto const name = w.tags()["name"];
    if (highway && name && (strcmp(highway, "residential") == 0 ||
                            strcmp(highway, "living_street") == 0)) {
      names_.emplace_back(w.tags()["name"]);
    }
  }

  std::vector<std::string> names_;
};

template <typename H>
void apply(osmium::io::File const& path, H& handler) {
  osmium::io::Reader reader(path);
  osmium::apply(reader, handler);
}

void extract(std::string const& input_path, std::ostream& out) {
  auto extractor = street_extractor();
  auto const path = osmium::io::File(input_path);
  apply(path, extractor);

  for (auto const& name : extractor.names_) {
    out << name << "\n";
  }
}

}  // namespace address_typeahead
