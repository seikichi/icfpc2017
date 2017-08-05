#include "game.h"
#include "strings.h"

using namespace picojson;

bool Game::Deserialize(const string &json) {
  return Deserialize(StringToJson(json));
}
bool Game::Deserialize(const picojson::object &json) {
  Clear();
  // {"punter" : p, "punters" : n, "map" : map}
  punter_id = json.at("punter").get<double>();
  punter_num = json.at("punters").get<double>();
  auto l_map = json.at("map").get<picojson::object>();
  map.Deserialize(l_map);
  if (json.count("settings")) {
    auto l_setting = json.at("settings").get<picojson::object>();
    setting.Deserialize(l_setting);
  }
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
  if (setting.Exist()) {
    picojson::object l_setting = setting.SerializeJson();
    l_game["settings"] = value(l_setting);
  }
  return l_game;
}
