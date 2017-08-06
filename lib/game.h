#pragma once
#include <string>
#include <boost/serialization/serialization.hpp>
#include "picojson.h"

#include "map.h"
#include "setting.h"
using namespace std;

class Game {
public:
  Game() { Clear(); }
  Game(const string &json) { Deserialize(json); }
  Game(const picojson::object &json) { Deserialize(json); }
  void Clear() {
    punter_id = -1;
    punter_num = -1;
    map.Clear();
    setting.Clear();
  }

  int PunterID() const { return punter_id; }
  int PunterNum() const { return punter_num; }
  const ::Map &Map() const { return map; }
  const ::Setting &Setting() const { return setting; }

  bool Deserialize(const string &json);
  bool Deserialize(const picojson::object &json);
  std::string SerializeString(bool prettify = false) const;
  picojson::object SerializeJson() const;
private:
  int punter_id;
  int punter_num;
  ::Map map;
  ::Setting setting;

  friend class boost::serialization::access;
  template <class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar & punter_id;
      ar & punter_num;
      ar & map;
      ar & setting;
    }
};
