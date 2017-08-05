#include "map.h"
#include <assert.h>
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
  // TODO サイズが大きい場合の対応
  dists = vector<vector<int>>(Size(), vector<int>(Size(), 1 << 29));
  for (int i = 0; i < Size(); i++) { dists[i][i] = 0; }
  for (const auto &es : graph) {
    for (const auto &e : es) {
      dists[e.src][e.dest] = 1;
    }
  }
  for (int k = 0; k < Size(); k++) {
    for (int i = 0; i < Size(); i++) {
      for (int j = 0; j < Size(); j++) {
        dists[i][j] = min(dists[i][j], dists[i][k] + dists[k][j]);
      }
    }
  }
}

Error MapState::ApplyMove(const Map& map, int punter_id, Move move) {
  if (move.Type() == MoveType::kClaim) {
    int src = map.SiteID(move.Source());
    int dest = map.SiteID(move.Target());
    for (const Edge& e : map.Graph()[src]) {
      if (e.dest == dest) {
        if (edge2pid[e.id] != -1) {
          fprintf(stderr, "Punter %d claimed river (%d -> %d), but it already claimed by punter %d.\n",
                  punter_id, src, dest, edge2pid[e.id]);
          return kBad;
        }
        edge2pid[e.id] = punter_id;
        return kOk;
      }
    }
    fprintf(stderr, "Punter %d claimed river (%d -> %d), but there are no such rivers.\n",
            punter_id, src, dest);
    return kBad;
  } else {
    return kOk;
  }
}

ostream& operator<<(ostream& stream, const MapState& map_state) {
  stream << "MapState( edge2pid = " << map_state.edge2pid << " )";
  return stream;
}
