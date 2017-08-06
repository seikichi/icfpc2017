#pragma once
#include <iostream>
#include <cassert>
#include <vector>
#include <boost/serialization/serialization.hpp>
#include "picojson.h"
#include "strings.h"

enum class MoveType {
  kClaim,
  kPass,
  kSplurge,
};

class Move {
public:
  Move() : type(MoveType::kPass), punter_id(-1), source(-1), target(-1) {;}
  static Move Claim(int punter_id, int source, int target) {
    return Move(MoveType::kClaim, punter_id, source, target, std::vector<int>());
  }

  static Move Pass(int punter_id) {
    return Move(MoveType::kPass, punter_id, -1, -1, std::vector<int>());
  }

  static Move Splurge(int punter_id, const std::vector<int> &route) {
    return Move(MoveType::kSplurge, punter_id, -1, -1, route);
  }


  MoveType Type() const { return type; }
  int PunterID() const { return punter_id; }
  int Source() const { return source; }
  int Target() const { return target; }
  const std::vector<int> &Route() const { return route; }

  std::string SerializeString(bool prettify = false) const {
    picojson::object o = SerializeJson();
    return picojson::value(o).serialize(prettify);
  }
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
    } else if (type == MoveType::kSplurge) {
      picojson::object j;
      j["punter"] = picojson::value((double)punter_id);
      picojson::array l_route = picojson::array();
      for (const auto &id : route) {
        l_route.emplace_back(picojson::value((double)id));
      }
      j["route"] = picojson::value(l_route);

      picojson::object k;
      k["splurge"] = picojson::value(j);
      return k;
    } else {
      assert(false);
    }
  }

  static Move Deserialize(const std::string& json) {
    return Deserialize(StringToJson(json));
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
    } else if (json.find("splurge") != json.end()) {
      auto o = json.at("splurge").get<picojson::object>();
      int punter_id = (int)o.at("punter").get<double>();
      std::vector<int> route;
      picojson::array l_route = o.at("route").get<picojson::array>();
      for (const picojson::value &v : l_route) {
        route.emplace_back((int)v.get<double>());
      }
      return Move::Splurge(punter_id, route);
    } else {
      assert(false);
    }
  }

private:
  Move(MoveType type, int punter_id, int source, int target, const std::vector<int> &route):
    type(type), punter_id(punter_id), source(source), target(target), route(route) {}

private:
  MoveType type;
  int punter_id;
  int source;
  int target;
  std::vector<int> route;

  friend class boost::serialization::access;
  template <class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar & type;
      ar & punter_id;
      ar & source;
      ar & target;
      ar & route;
    }
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
