using namespace picojson;

bool Map::Init(string json) {
  string err = parse(v, json);
  if (!err.empty()) {
    cerr << err << endl;
    return false;
  }

  auto o = v.get<object>();
  return Init(o);
}
bool Map::Init(picojson::object json) {
  Clear();
  // = {"sites" : [Site], "rivers" : [River], "mines" : [SiteId]}
  auto l_sites = json["sites"].get<array>();
  for (const value &v : l_sites) {
    // {"id" : Nat }
    auto o = v.get<object>();
    int id = (int)o.["id"].get<double>();
    int index = site_id_map.size();
    site_id_map[id] = index;
    site_id_rev_map[index] = id;
    sites.emplace_back(index)
  }
  graph = Graph(size_id_map.size());
  auto l_rivers = json["rivers"].get<array>();
  for (const value &v : l_rivers) {
    // {"source" : SiteId, "target" : SiteId}
    auto o = v.get<object>();
    int source = (int)o["source"].get<double>();
    int target = (int)o["target"].get<double>();
    source = site_id_map[source];
    target = site_id_map[target];
    graph[source].emplace_back(source, target);
  }
  auto l_mines = json["mines"].get<array>();
  for (const value &v : l_mines) {
    // [SiteId]
    int id = (int)v.get<double>();
    id = site_id_map[id];
    sites[id].setMine(true);
  }
  return true;
}
