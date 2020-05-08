#include "address-typeahead/extract.h"

#include "cereal/archives/binary.hpp"

#include "address-typeahead/common.h"
#include "address-typeahead/extractor.h"
#include "address-typeahead/serialization.h"

namespace address_typeahead {

void extract(std::string const& input_path, std::ofstream& out) {
  address_typeahead::extract_options options;
  options.whitelist_add("name");
  options.whitelist_add("highway");
  options.whitelist_add("addr:street");
  options.whitelist_add("addr:housenumber");

  options.blacklist_add("tram");
  options.blacklist_add("power");
  options.blacklist_add("train");
  options.blacklist_add("aeroway");
  options.blacklist_add("waterway");
  options.blacklist_add("railway");
  options.blacklist_add("barrier");
  options.blacklist_add("service");
  options.blacklist_add("traffic_sign");
  options.blacklist_add("natural", "tree");
  options.blacklist_add("building", "garage");
  options.blacklist_add("amenity", "parking");
  options.blacklist_add("landuse", "garages");
  options.blacklist_add("landuse", "farmland");
  options.blacklist_add("highway", "service");
  options.blacklist_add("highway", "bus_stop");
  options.blacklist_add("amenity", "waste_disposal");
  options.approximation_lvl_ = address_typeahead::APPROX_LVL_3;

  auto const context = address_typeahead::extract(input_path, options);
  {
    cereal::BinaryOutputArchive oa(out);
    oa(context);
  }
}

}  // namespace address_typeahead