#pragma once
#include <memory>
struct UnionFind {
  int n;
  std::unique_ptr<int[]> parent;
  UnionFind(int n) : n(n) {
    parent = std::make_unique<int[]>(n);
    memset(parent.get(), -1, sizeof(int) * n);
  }
  bool UnionSet(int x, int y) {
    x = Root(x); y = Root(y);
    if (x == y) { return false; }
    if (parent[y] < parent[x]) { swap(x, y); }
    parent[x] += parent[y];
    parent[y] = x;
    return true;
  }
  bool FindSet(int x, int y) {
    return Root(x) == Root(y);
  }
  int Root(int x) { return parent[x] < 0 ? x : parent[x] = Root(parent[x]); }
  int Size(int x) { return -parent[Root(x)]; }
};
