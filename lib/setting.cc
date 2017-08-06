#include "setting.h"
#include "strings.h"

using namespace picojson;

bool Setting::Deserialize(const string &json) {
  return Deserialize(StringToJson(json));
}
bool Setting::Deserialize(const picojson::object &json) {
  Clear();
  exist = true;
  // {"futures" : true, "splurges": true}
  if (json.count("futures")) {
    futures = json.at("futures").get<bool>();
  }
  if (json.count("splurges")) {
    splurges = json.at("splurges").get<bool>();
  }
  return true;
}

std::string Setting::SerializeString(bool prettify) const {
  object o = SerializeJson();
  return picojson::value(o).serialize(prettify);
}
picojson::object Setting::SerializeJson() const {
  picojson::object l_setting;
  if (futures) {
    l_setting["futures"] = picojson::value(true);
  }
  if (splurges) {
    l_setting["splurges"] = picojson::value(true);
  }
  return l_setting;
}
