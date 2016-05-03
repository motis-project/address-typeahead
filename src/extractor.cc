#include <fstream>
#include <ostream>

#include "osmium/handler.hpp"
#include "osmium/io/pbf_input.hpp"
#include "osmium/memory/buffer.hpp"
#include "osmium/osm.hpp"
#include "osmium/visitor.hpp"

namespace address_typeahead {

class street_extractor : public osmium::handler::Handler {
public:
  void way(osmium::Way const& w) {
    auto const highway = w.tags()["highway"];
    auto const name = w.tags()["name"];
    if (highway && strcmp(highway, "residential") == 0 && name) {
      names_.emplace_back(w.tags()["name"]);
    }
  }

  std::vector<std::string> names_;
};

template <typename H>
void apply(std::string const& pbf, H& handler) {
  osmium::io::Reader reader(pbf);
  osmium::apply(reader, handler);
}

void extract(std::string const& input_path, std::string const& output_path) {
  street_extractor node_counter;
  apply(input_path, node_counter);

  std::ofstream out(output_path);
  for (auto const& name : node_counter.names_) {
    out << name << "\n";
  }
}

}  // namespace address_typeahead
