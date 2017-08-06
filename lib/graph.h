#pragma once
#include <sstream>
#include <vector>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

using namespace std;

struct Edge {
  int id;
  int src;
  int dest;

  Edge() : id(-1), src(-1), dest(-1) {;}
  Edge(int id, int src, int dest) : id(id), src(src), dest(dest) {;}
  bool operator<(const Edge &rhs) const {
    if (src != rhs.src) { return src < rhs.src; }
    return dest < rhs.dest;
  }
  ostream &operator<<(ostream &os) const {
    os << "{ ";
    // os << "\"id\":" << id << ", ";
    os << "\"src\":" << src << ", ";
    os << "\"dest\":" << dest << ", ";
    os << " }";
    return os;
  }

  friend class boost::serialization::access;
  template <class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar & id;
      ar & src;
      ar & dest;
    }
};
typedef vector<Edge> Edges;
typedef vector<Edges> Graph;

int Maxflow(const Graph &g, int s, int t);
