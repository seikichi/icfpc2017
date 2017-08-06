#include "map.h"
#include "gtest/gtest.h"

void AddEdge(Graph &g, int &e, int from, int to) {
  g[from].push_back(Edge(e, from, to));
  g[to].push_back(Edge(e, to, from));
  e++;
}

TEST(graph, Maxflow) {
  Graph g(4);
  int e = 0;
  AddEdge(g, e, 0, 1);
  AddEdge(g, e, 1, 2);
  ASSERT_EQ(Maxflow(g, 0, 1), 1);
  ASSERT_EQ(Maxflow(g, 1, 0), 1);
  ASSERT_EQ(Maxflow(g, 0, 2), 1);
  ASSERT_EQ(Maxflow(g, 2, 0), 1);
  ASSERT_EQ(Maxflow(g, 0, 3), 0);
  ASSERT_EQ(Maxflow(g, 3, 0), 0);

  AddEdge(g, e, 2, 3);
  AddEdge(g, e, 1, 3);
  ASSERT_EQ(Maxflow(g, 0, 3), 1);
  ASSERT_EQ(Maxflow(g, 3, 0), 1);
  ASSERT_EQ(Maxflow(g, 1, 3), 2);
  ASSERT_EQ(Maxflow(g, 3, 1), 2);
}
