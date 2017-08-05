#pragma once
#include <map>
#include "picojson.h"

#include "graph.h"
#include "site.h"
using namespace std;

class Map {
public:
  Map() { Clear(); }
  Map(const std::string &json) { Deserialize(json); }
  Map(const picojson::object &json) { Deserialize(json); }
  void Clear() {
    sites.clear();
    graph.clear();
    site_id_map.clear();
    dists.clear();
  }
  int Size() const { return site_id_map.size(); }
  const vector<Site> &Sites() const { return sites; }
  const vector<Edges> &Graph() const { return graph; }

  int Dist(int from, int to) const { return dists[from][to]; }
  int SiteID(int original_id) const { return site_id_map.at(original_id); }

  bool Deserialize(const string &json);
  bool Deserialize(const picojson::object &json);
  std::string SerializeString(bool prettify = false) const;
  picojson::object SerializeJson() const;

private:
  void InitDists();
  vector<Site> sites;
  vector<Edges> graph;
  map<int, int> site_id_map;
  vector<vector<int>> dists;
};
