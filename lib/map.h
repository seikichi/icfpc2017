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
  int EdgeNum() const { return edge_num; }

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
  int edge_num;

  friend class boost::serialization::access;
  template <class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar & sites;
      ar & graph;
      ar & mines;
      ar & site_id_map;
      ar & dists;
      ar & edge_num;
    }
};

class MapState {
public:
  MapState() {;}
  explicit MapState(const Map& map, int punter_num) { Clear(map, punter_num); }

  void Clear(const Map& map, int punter_num) {
    edge2pid.resize(map.EdgeNum(), -1);
    pass_count.resize(punter_num, 0);
  }

  Error ApplyMove(const Map& map, Move move, bool verbose=false);
  void SetEdgeImportances(const vector<vector<int>> &importances) { edge_importances = importances; }

  // Get the id of the punter who claimed the specified eedge.
  // If no punters claimed the edge, returns -1.
  int Claimer(int edge_id) const {
    return edge2pid[edge_id];
  }
  const vector<vector<int>> &EdgeImportances() const { return edge_importances; }

  picojson::object SerializeJson() const;
  void Deserialize(const picojson::object& json);

private:
  vector<int> edge2pid;
  vector<vector<int>> edge_importances;
  vector<int> pass_count;

  friend ostream& operator<<(ostream& stream, const MapState& map_state);

  friend class boost::serialization::access;
  template <class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar & edge2pid;
      ar & pass_count;
      ar & edge_importances;
    }
};

ostream& operator<<(ostream& stream, const MapState& map_state);
