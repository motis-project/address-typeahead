#include <boost/serialization/base_object.hpp>
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
void serialize(Archive& ar, address_typeahead::area& a,
               unsigned int const version) {
  ar& a.name_;
  ar& a.level_;
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::location& l,
               unsigned int const version) {
  ar& l.coordinates_;
  ar& l.name_;
  ar& l.areas_;
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::house_number& hn,
               unsigned int const version) {
  ar& hn.name_;
  ar& hn.coordinates_;
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::street& s,
               unsigned int const version) {
  ar& s.name_;
  ar& s.house_numbers_;
  ar& s.areas_;
}

template <class Archive>
void serialize(Archive& ar, address_typeahead::typeahead_context& context,
               unsigned int const version) {
  ar& context.places_;
  ar& context.streets_;
  ar& context.areas_;
}

}  // namespace serialization
}  // namespace boost
