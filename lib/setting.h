#pragma once
#include <string>
#include "picojson.h"

using namespace std;

class Setting {
public:
  Setting() { Clear(); }
  Setting(const string &json) { Deserialize(json); }
  Setting(const picojson::object &json) { Deserialize(json); }
  void Clear() {
    exist = false;
    futures = false;
  }

  bool Exist() const { return exist; }
  bool Futures() const { return futures; }

  bool Deserialize(const string &json);
  bool Deserialize(const picojson::object &json);
  std::string SerializeString(bool prettify = false) const;
  picojson::object SerializeJson() const;
private:
  bool exist;
  bool futures;
};
