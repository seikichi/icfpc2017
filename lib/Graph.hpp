#include <vector>

struct Edge {
  int src;
  int dest;
  Edge(int src, int dest) : src(src), dest(dest) {;}
  bool operator<(const Edge &rhs) const {
    if (src != rhs.src) { return src < rhs.src; }
    return dest < rhs.dest;
  }
  ostream &operator<<(ostream &os) const {
    os << "{ ";
    os << "\"src\":" << src << ", ";
    os << "\"dest\":" << dest << ", ";
    os << " }";
    return os;
  }
};
typedef vector<Edge> Edges;
typedef vector<Edges> Graph;
// typedef vector<Weight> Array;
// typedef vector<Array> Matrix;
