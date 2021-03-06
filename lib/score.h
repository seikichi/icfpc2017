#pragma once

#include <stdint.h>
#include <string>
#include <boost/serialization/serialization.hpp>
#include "picojson.h"
using namespace std;

class Score {
public:
  Score() { Clear(); }
  Score(int punter_id, int64_t score):
    punter_id(punter_id), score(score) {}
  Score(const std::string &json) { Deserialize(json); }
  Score(const picojson::object &json) { Deserialize(json); }
  void Clear() {
    punter_id = -1;
    score = -1;
  }
  int PunterID() const { return punter_id; }
  int64_t Get() const { return score; }

  bool Deserialize(const string &json);
  bool Deserialize(const picojson::object &json);
  std::string SerializeString(bool prettify = false) const;
  picojson::object SerializeJson() const;

private:
  int punter_id;
  int64_t score;

  friend class boost::serialization::access;
  template <class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar & punter_id;
      ar & score;
    }
};
