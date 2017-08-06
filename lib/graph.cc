#include "graph.h"
#include <queue>
using namespace std;

typedef vector<int> Array;
typedef vector<Array> Matrix;

int ConvertIndex(const Edge &edge) {
  if (edge.src < edge.dest) { return edge.id * 2; }
  return edge.id * 2 + 1;
}

int augment(const Graph &g, Array &capacity, const vector<int> &level, vector<bool> &finished, int from, int t, int cur) {
  if (from == t || cur == 0) { return cur; }
  if (finished[from]) { return 0; }
  for (Edges::const_iterator it = g[from].begin(); it != g[from].end(); it++) {
    int to = it->dest;
    if (level[to] != level[from] + 1) { continue; }
    int f = augment(g, capacity, level, finished, to, t, min(cur, capacity[ConvertIndex(*it)]));
    if (f > 0) {
      capacity[ConvertIndex(*it)] -= f;
      capacity[ConvertIndex(*it)^1] += f;
      return f;
    }
  }
  finished[from] = true;
  return 0;
}

// index^1 is reverse edge
int Maxflow(const Graph &g, int s, int t) {
  int e = 0;
  for (const auto &edges : g) {
    for (const auto &edge : edges) {
      e = max(e, edge.id + 1);
    }
  }
  e *= 2;
  int n = g.size();
  Array capacity(e);
  for (int from = 0; from < n; from++) {
    for (Edges::const_iterator it = g[from].begin(); it != g[from].end(); it++) {
      capacity[ConvertIndex(*it)] += 1;
    }
  }
  int ans = 0;
  while (true) {
    vector<int> level(n, -1);
    level[s] = 0;
    queue<int> que;
    que.push(s);
    for (int d = n; !que.empty() && level[que.front()] < d; ) {
      int from = que.front();
      que.pop();
      if (from == t) { d = level[from]; }
      for (Edges::const_iterator it = g[from].begin(); it != g[from].end(); it++) {
        int to = it->dest;
        if (capacity[ConvertIndex(*it)] > 0 && level[to] == -1) {
          que.push(to);
          level[to] = level[from] + 1;
        }
      }
    }
    vector<bool> finished(n);
    bool end = true;
    while (true) {
      int f = augment(g, capacity, level, finished, s, t, 2000000000ll);
      if (f == 0) { break; }
      ans += f;
      end = false;
    }
    if (end) { break; }
  }
  return ans;
}
