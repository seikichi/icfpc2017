#include <map>
#include "picojson.h"

#include "graph.h"
#include "site.h"
using namespace std;

class Map {
public:
  Map() { Clear(); }
  Map(const std::string &json) { Init(json); }
  Map(const picojson::object &json) { Init(json); }
  void Clear() {
    sites.clear();
    graph.clear();
    site_id_map.clear();
    site_id_rev_map.clear();
    dists.clear();
  }
  bool Init(const string &json);
  bool Init(const picojson::object &json);
  int Size() const { return site_id_map.size(); }
  int Dist(int from, int to) const { return dists[from][to]; }
  const vector<Site> &GetSites() const { return sites; }
  const vector<Edges> &GetGraph() const { return graph; }

  std::string SerializeString(bool prettify = false) const;
  picojson::object SerializeJson() const;

private:
  void InitDists();
  vector<Site> sites;
  vector<Edges> graph;
  map<int, int> site_id_map;
  map<int, int> site_id_rev_map;
  vector<vector<int>> dists;
};
