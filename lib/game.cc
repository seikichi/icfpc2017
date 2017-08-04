#include "game.h"

using namespace picojson;

bool Game::Deserialize(const string &json) {
  value v;
  string err = parse(v, json);
  if (!err.empty()) {
    cerr << err << endl;
    return false;
  }

  auto o = v.get<object>();
  return Deserialize(o);
}

bool Game::Deserialize(const picojson::object &json) {
  Clear();
  // {"punter" : p, "punters" : n, "map" : map}
  punter_id = json.at("punter").get<double>();
  punter_num = json.at("punters").get<double>();
  auto l_map = json.at("map").get<picojson::object>();
  map.Deserialize(l_map);
  return true;
}

std::string Game::SerializeString(bool prettify) const {
  object o = SerializeJson();
  return picojson::value(o).serialize(prettify);
}
picojson::object Game::SerializeJson() const {
  picojson::object l_game;
  picojson::object l_map = map.SerializeJson();
  l_game["punter"] = value((double)punter_id);
  l_game["punters"] = value((double)punter_num);
  l_game["map"] = value(l_map);
  return l_game;
}
