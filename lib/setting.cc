#include "setting.h"

using namespace picojson;

bool Setting::Deserialize(const string &json) {
  value v;
  string err = parse(v, json);
  if (!err.empty()) {
    cerr << err << endl;
    return false;
  }

  auto o = v.get<object>();
  return Deserialize(o);
}

bool Setting::Deserialize(const picojson::object &json) {
  Clear();
  exist = true;
  // {"futures" : true}
  if (json.count("futures")) {
    futures = json.at("futures").get<bool>();
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
  return l_setting;
}
