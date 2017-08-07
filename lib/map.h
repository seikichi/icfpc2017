#pragma once
#include <map>
#include <iostream>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/map.hpp>
#include "picojson.h"

#include "graph.h"
#include "site.h"
#include "error.h"
#include "move.h"
using namespace std;

class Map {
public:
  Map() { Clear(); }
  Map(const std::string &json) { Deserialize(json); }
  Map(const picojson::object &json) { Deserialize(json); }
  void Clear() {
    sites.clear();
    graph.clear();
    mines.clear();
    site_id_map.clear();
    dists.clear();
  }
  int Size() const { return site_id_map.size(); }
  const vector<Site> &Sites() const { return sites; }
  const vector<Site> &Mines() const { return mines; }
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
  vector<Site> mines;
  vector<Edges> graph;
  map<int, int> site_id_map;
  vector<vector<int>> dists;

  friend class boost::serialization::access;
  template <class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar & sites;
      ar & graph;
      ar & mines;
      ar & site_id_map;
      ar & dists;
    }
};

class MapState {
public:
  MapState() {;}
  explicit MapState(const Map& map) { Clear(map); }

  void Clear(const Map& map) {
    size_t n_edges = 0;
    for (const auto& es : map.Graph()) {
      n_edges += es.size();
    }
    n_edges /= 2;

    edge2pid.resize(n_edges, -1);
    pass_count = 0;
  }

  Error ApplyMove(const Map& map, Move move, bool verbose=false);

  // Get the id of the punter who claimed the specified eedge.
  // If no punters claimed the edge, returns -1.
  int Claimer(int edge_id) const {
    return edge2pid[edge_id];
  }

  picojson::object SerializeJson() const;
  void Deserialize(const picojson::object& json);

private:
  vector<int> edge2pid;
  int pass_count;

  friend ostream& operator<<(ostream& stream, const MapState& map_state);

  friend class boost::serialization::access;
  template <class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar & edge2pid;
      ar & pass_count;
    }
};

ostream& operator<<(ostream& stream, const MapState& map_state);
