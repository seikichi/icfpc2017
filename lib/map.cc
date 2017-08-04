#include "map.h"

using namespace picojson;

bool Map::Init(const string &json) {
  value v;
  string err = parse(v, json);
  if (!err.empty()) {
    cerr << err << endl;
    return false;
  }

  auto o = v.get<object>();
  return Init(o);
}
bool Map::Init(const picojson::object &json) {
  Clear();
  // {"sites" : [Site], "rivers" : [River], "mines" : [SiteId]}
  auto l_sites = json.at("sites").get<picojson::array>();
  for (const value &v : l_sites) {
    // {"id" : Nat }
    auto o = v.get<object>();
    int id = (int)o.at("id").get<double>();
    int index = site_id_map.size();
    site_id_map[id] = index;
    site_id_rev_map[index] = id;
    sites.emplace_back(index);
  }
  const int n = site_id_map.size();
  graph = Graph(n);
  auto l_rivers = json.at("rivers").get<picojson::array>();
  for (const value &v : l_rivers) {
    // {"source" : SiteId, "target" : SiteId}
    auto o = v.get<object>();
    int source = (int)o.at("source").get<double>();
    int target = (int)o.at("target").get<double>();
    source = site_id_map[source];
    target = site_id_map[target];
    graph[source].emplace_back(source, target);
  }
  auto l_mines = json.at("mines").get<picojson::array>();
  for (const value &v : l_mines) {
    // [SiteId]
    int id = (int)v.get<double>();
    id = site_id_map[id];
    sites[id].setMine(true);
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
    l_site["id"] = value((double)site_id_rev_map.at(site.id));
    l_sites.push_back(value(l_site));
    if (site.is_mine) {
      l_mines.push_back(value((double)site_id_rev_map.at(site.id)));
    }
  }
  for (const auto &es : graph) {
    for (const auto &e : es) {
      picojson::object l_river;
      l_river["source"] = value((double)site_id_rev_map.at(e.src));
      l_river["dest"] = value((double)site_id_rev_map.at(e.dest));
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
