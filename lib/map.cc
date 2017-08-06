#include "map.h"
#include <assert.h>
#include <queue>
#include "strings.h"
#include "cout.h"

using namespace picojson;

bool Map::Deserialize(const string &json) {
  return Deserialize(StringToJson(json));
}
bool Map::Deserialize(const picojson::object &json) {
  Clear();
  // {"sites" : [Site], "rivers" : [River], "mines" : [SiteId]}
  auto l_sites = json.at("sites").get<picojson::array>();
  for (const value &v : l_sites) {
    // {"id" : Nat }
    auto o = v.get<object>();
    int id = (int)o.at("id").get<double>();
    int index = site_id_map.size();
    site_id_map[id] = index;
    sites.emplace_back(index, id);
  }
  const int n = site_id_map.size();
  graph = ::Graph(n);
  auto l_rivers = json.at("rivers").get<picojson::array>();
  int edge_num = 0;
  for (const value &v : l_rivers) {
    // {"source" : SiteId, "target" : SiteId}
    auto o = v.get<object>();
    int source = (int)o.at("source").get<double>();
    int target = (int)o.at("target").get<double>();
    source = site_id_map[source];
    target = site_id_map[target];
    graph[source].emplace_back(edge_num, source, target);
    graph[target].emplace_back(edge_num, target, source);
    edge_num++;
  }
  auto l_mines = json.at("mines").get<picojson::array>();
  for (const value &v : l_mines) {
    // [SiteId]
    int id = (int)v.get<double>();
    int index = site_id_map[id];
    sites[index].setMine(true);
    assert(id == sites[index].original_id);
  }
  InitDists();
  return true;
}

string Map::SerializeString(bool prettify) const {
  object o = SerializeJson();
  return picojson::value(o).serialize(prettify);
}
picojson::object Map::SerializeJson() const {
  picojson::object l_map;
  picojson::array l_sites, l_rivers, l_mines;
  for (const auto &site : sites) {
    picojson::object l_site;
    l_site["id"] = value((double)site.original_id);
    l_sites.push_back(value(l_site));
    if (site.is_mine) {
      l_mines.push_back(value((double)site.original_id));
    }
  }
  for (const auto &es : graph) {
    for (const auto &e : es) {
      if (e.src >= e.dest) { continue; }
      picojson::object l_river;
      l_river["source"] = value((double)sites[e.src].original_id);
      l_river["target"] = value((double)sites[e.dest].original_id);
      l_rivers.push_back(value(l_river));
    }
  }
  l_map["sites"] = value(l_sites);
  l_map["rivers"] = value(l_rivers);
  l_map["mines"] = value(l_mines);
  return l_map;
}



void Map::InitDists() {
  dists = vector<vector<int>>(Size(), vector<int>(Size(), 1 << 29));
  for (int start = 0; start < Size(); start++) {
    dists[start][start] = 0;
    queue<pair<int, int>> que;
    que.push(make_pair(start, 0));
    while (!que.empty()) {
      int from = que.front().first;
      int d = que.front().second;
      que.pop();
      for (const auto &edge : graph[from]) {
        int to = edge.dest;
        int nd = d + 1;
        if (dists[start][to] <= nd) { continue; }
        dists[start][to] = nd;
        que.push(make_pair(to, nd));
      }
    }
  }
}

Error MapState::ApplyMove(const Map& map, Move move, bool verbose) {
  if (move.Type() == MoveType::kClaim) {
    pass_count = 0;
    int src = map.SiteID(move.Source());
    int dest = map.SiteID(move.Target());
    for (const Edge& e : map.Graph()[src]) {
      if (e.dest == dest) {
        if (edge2pid[e.id] != -1) {
          if (verbose) {
            fprintf(stderr, "Punter %d claimed river (%d -> %d), but it already claimed by punter %d.\n",
                    move.PunterID(), src, dest, edge2pid[e.id]);
          }
          return kBad;
        }
        edge2pid[e.id] = move.PunterID();
        return kOk;
      }
    }
    if (verbose) {
      fprintf(stderr, "Punter %d claimed river (%d -> %d), but there are no such rivers.\n",
              move.PunterID(), src, dest);
    }
    return kBad;
  } else if (move.Type() == MoveType::kSplurge) {
    pass_count = 0;
    if (pass_count < (int)move.Route().size() - 1) {
      if (verbose) {
        fprintf(stderr, "Punter %d splurges %d length, but he has only %d pass count.\n",
            move.PunterID(), (int)move.Route().size(), pass_count);
      }
      return kBad;
    }
    const vector<int> prev_edge2pid = edge2pid;
    for (int i = 0; i < (int)move.Route().size() - 1; i++) {
      Move one_move = Move::Claim(move.PunterID(), move.Route()[i], move.Route()[i + 1]);
      Error nret = ApplyMove(map, one_move, verbose);
      if (nret == kBad) {
        // rollback
        edge2pid = prev_edge2pid;
        return kBad;
      }
    }
    return kOk;
  } else {
    pass_count++;
    return kOk;
  }
}

ostream& operator<<(ostream& stream, const MapState& map_state) {
  stream << "MapState( edge2pid = " << map_state.edge2pid << " )";
  return stream;
}

picojson::array IntVectorToJson(const std::vector<int>& v) {
  picojson::array a;
  for (auto& e : v) {
    a.push_back(picojson::value((double)e));
  }
  return a;
}

picojson::object MapState::SerializeJson() const {
  auto a = IntVectorToJson(edge2pid);

  picojson::object o;
  o["edge2pid"] = picojson::value(a);

  return o;
}

void MapState::Deserialize(const picojson::object& json) {
  edge2pid.clear();
  for (auto& e : json.at("edge2pid").get<picojson::array>()) {
    edge2pid.push_back(e.get<double>());
  }
}
