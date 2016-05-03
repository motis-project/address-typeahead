#include <iostream>

#include "osmium/handler.hpp"
#include "osmium/osm.hpp"
#include "osmium/visitor.hpp"
#include "osmium/io/pbf_input.hpp"
#include "osmium/memory/buffer.hpp"

class node_counter : public osmium::handler::Handler {
public:
  void node(osmium::Node const&) {
    ++count;
  }
  size_t count = 0;
};

template<typename H>
void apply(std::string const& pbf, H& handler) {
  osmium::io::Reader reader(pbf);
  osmium::apply(reader, handler);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " file.osm.pbf\n";
    return 0;
  }

  node_counter node_counter;
  apply(argv[1], node_counter);

  std::cout << node_counter.count << " nodes found\n";
}
