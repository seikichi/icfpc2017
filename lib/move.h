#pragma once
#include <iostream>
#include <cassert>
#include "picojson.h"

enum class MoveType {
  kClaim,
  kPass,
};

class Move {
public:
  static Move Claim(int punter_id, int source, int target) {
    return Move(MoveType::kClaim, punter_id, source, target);
  }

  static Move Pass(int punter_id) {
    return Move(MoveType::kPass, punter_id, -1, -1);
  }

  MoveType Type() const { return type; }
  int PunterID() const { return punter_id; }
  int Source() const { return source; }
  int Target() const { return target; }

  picojson::object SerializeJson() const {
    if (type == MoveType::kClaim) {
      picojson::object j;
      j["punter"] = picojson::value((double)punter_id);
      j["source"] = picojson::value((double)source);
      j["target"] = picojson::value((double)target);

      picojson::object k;
      k["claim"] = picojson::value(j);
      return k;
    } else if (type == MoveType::kPass) {
      picojson::object j;
      j["punter"] = picojson::value((double)punter_id);

      picojson::object k;
      k["pass"] = picojson::value(j);
      return k;
    } else {
      assert(false);
    }
  }

  static Move Deserialize(const picojson::object& json) {
    if (json.find("claim") != json.end()) {
      auto o = json.at("claim").get<picojson::object>();
      int punter_id = (int)o.at("punter").get<double>();
      int source = (int)o.at("source").get<double>();
      int target = (int)o.at("target").get<double>();
      return Move::Claim(punter_id, source, target);
    } else if (json.find("pass") != json.end()) {
      auto o = json.at("pass").get<picojson::object>();
      int punter_id = (int)o.at("punter").get<double>();
      return Move::Pass(punter_id);
    } else {
      assert(false);
    }
  }

private:
  Move(MoveType type, int punter_id, int source, int target):
    type(type), punter_id(punter_id), source(source), target(target) {}

private:
  MoveType type;
  int punter_id;
  int source;
  int target;
};

inline std::ostream& operator<<(std::ostream& stream, const Move& move) {
  switch (move.Type()) {
  case MoveType::kClaim:
    stream << "Claim(" << move.Source() << ", " << move.Target() << ")";
    break;
  case MoveType::kPass:
    stream << "Pass";
    break;
  default:
    assert(false);
  }
  return stream;
}
