#pragma once
#include <string>
#include "picojson.h"

#include "map.h"
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
  }

  int PunterID() const { return punter_id; }
  int PunterNum() const { return punter_num; }
  const ::Map &Map() const { return map; }
  bool Deserialize(const string &json);
  bool Deserialize(const picojson::object &json);
  std::string SerializeString(bool prettify = false) const;
  picojson::object SerializeJson() const;
private:
  int punter_id;
  int punter_num;
  ::Map map;
};
