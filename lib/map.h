#include <map>
#include "picojson.h"

#include "graph.h"
#include "site.h"
using namespace std;

class Map {
public:
  Map() { Clear(); }
  Map(std::string json) { Init(json); }
  Map(picojson::object json) { Init(json); }
  void Clear() {
    sites.clear();
    graph.clear();
    site_id_map.clear();
    site_id_rev_map.clear();
    dists.clear();
  }
  bool Init(string json);
  bool Init(picojson::object json);
  int Size() const { return site_id_map.size(); }
  int Dist(int from, int to) const { return dists[from][to]; }
  const vector<Site> &getSites() const { return sites; }
  const vector<Edges> &getGraph() const { return graph; }

private:
  void InitDists();
  vector<Site> sites;
  vector<Edges> graph;
  map<int, int> site_id_map;
  map<int, int> site_id_rev_map;
  vector<vector<int>> dists;
};
