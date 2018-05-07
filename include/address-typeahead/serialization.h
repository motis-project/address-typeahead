#include <boost/serialization/base_object.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include "common.h"

namespace boost {
namespace serialization {

template <class Archive>
void save(Archive& ar, address_typeahead::point const& p,
          unsigned int const version) {
  ar& p.get<0>();
  ar& p.get<1>();
}

template <class Archive>
void load(Archive& ar, address_typeahead::point& p,
          unsigned int const version) {
  int32_t x, y;
  ar& x;
  ar& y;
  p.set<0>(x);
  p.set<1>(y);
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::point& p,
               unsigned int const version) {
  split_free(ar, p, version);
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::ring& r,
               unsigned int const version) {
  ar& static_cast<std::vector<address_typeahead::point>&>(r);
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::polygon& poly,
               unsigned int const version) {
  ar& poly.outer();
  ar& poly.inners();
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::box& b,
               unsigned int const version) {
  ar& b.min_corner();
  ar& b.max_corner();
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::value& val,
               unsigned int const version) {
  ar& val.first;
  ar& val.second;
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::multi_polygon& mpoly,
               unsigned int const version) {
  ar& static_cast<std::vector<address_typeahead::polygon>&>(mpoly);
}

template <class Archive>
void save(Archive& ar, boost::geometry::index::rtree<
                           address_typeahead::value,
                           boost::geometry::index::linear<16>> const& rtr,
          unsigned int const version) {
  std::vector<address_typeahead::value> values;
  values.resize(rtr.size());

  for (auto const& val : rtr) {
    values.emplace_back(val);
  }

  ar& values;
}

template <class Archive>
void load(
    Archive& ar,
    boost::geometry::index::rtree<address_typeahead::value,
                                  boost::geometry::index::linear<16>>& rtr,
    unsigned int const version) {
  std::vector<address_typeahead::value> values;
  ar& values;
  rtr.insert(values.begin(), values.end());
}

template <class Archive>
void serialize(
    Archive& ar,
    boost::geometry::index::rtree<address_typeahead::value,
                                  boost::geometry::index::linear<16>>& rtr,
    unsigned int const version) {
  split_free(ar, rtr, version);
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::area& a,
               unsigned int const version) {
  ar& a.name_;
  ar& a.level_;
  ar& a.mpolygon_;
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::address& a,
               unsigned int const version) {
  ar& a.coordinates_;
  ar& a.house_number_;
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::place& p,
               unsigned int const version) {
  ar& p.name_;
  ar& p.addresses_;
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::typeahead_context& context,
               unsigned int const version) {
  ar& context.places_;
  ar& context.areas_;
  ar& context.rtree_;
}

}  // namespace serialization
}  // namespace boost
