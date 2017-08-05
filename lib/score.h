#pragma once

#include <stdint.h>
#include <string>
#include "picojson.h"
using namespace std;

class Score {
public:
  Score() { Clear(); }
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
};
