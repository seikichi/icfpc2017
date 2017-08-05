#include "score.h"
#include <assert.h>
#include "strings.h"

using namespace picojson;

bool Score::Deserialize(const string &json) {
  return Deserialize(StringToJson(json));
}
bool Score::Deserialize(const picojson::object &json) {
  Clear();
  punter_id = json.at("punter").get<double>();
  score = json.at("score").get<double>();
  return true;
}

string Score::SerializeString(bool prettify) const {
  object o = SerializeJson();
  return picojson::value(o).serialize(prettify);
}
picojson::object Score::SerializeJson() const {
  picojson::object l_score;
  l_score["punter"] = value((double)punter_id);
  l_score["score"] = value((double)score);
  return l_score;
}
