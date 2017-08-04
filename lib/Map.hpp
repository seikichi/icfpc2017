#include <picojson/picojson.h>

#include "Graph.hpp"

class Map {
public:
  Map() {;}
  Map(string json) { Init(json); }
  Map(picojson::object json) { Init(Json); }
  void Clear() {
    site_id_map.clear();
    site_id_rev_map.clear();
    sites.clear();
    graph.clear();
  }
  bool Init(string json);
  bool Init(picojson::object json);

private:
  map<int, int> site_id_map;
  map<int, int> site_id_rev_map;
  vector<Site> sites;
  vector<Edges> graph;
};
