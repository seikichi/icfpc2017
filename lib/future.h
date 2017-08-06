#pragma once
#include <assert.h>
#include <string>
#include <boost/serialization/serialization.hpp>
#include "picojson.h"
#include "site.h"

using namespace std;

class Future {
public:
  Future() { Clear(); }
  // Future(const string &json) { Deserialize(json); }
  // Future(const picojson::object &json) { Deserialize(json); }
  void Clear() {
    source = Site();
    target = Site();
  }
  const Site &Source() const { return source; }
  const Site &Target() const { return target; }

  void SetSource(const Site &site) { assert(source.is_mine); source = site; }
  void SetTarget(const Site &site) { target = site; }

  bool Deserialize(const string &json);
  bool Deserialize(const picojson::object &json);
  std::string SerializeString(bool prettify = false) const;
  picojson::object SerializeJson() const;
private:
  Site source;
  Site target;

  friend class boost::serialization::access;
  template <class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar & source;
      ar & target;
    }
};
