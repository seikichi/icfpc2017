#include "map.h"
#include "gtest/gtest.h"

const string sample_json = R"(
{
  "sites": [
    {"id":0},
    {"id":1}
  ],
  "rivers": [
    {"source":0, "target":1}
  ],
  "mines": [
    0
  ]
}
)";

TEST(map, declear) {
  Map field;
  field.Clear();
}

TEST(map, Deseralize) {
  Map field;
  bool ret = field.Deserialize(sample_json);
  ASSERT_TRUE(ret);
  const vector<Site> &sites = field.Sites();
  const Graph &g = field.Graph();

  ASSERT_EQ(sites.size(), 2);
  ASSERT_TRUE(sites[0].is_mine);
  ASSERT_FALSE(sites[1].is_mine);

  ASSERT_EQ(g.size(), 2);
  ASSERT_EQ(g[0].size(), 1);
  ASSERT_EQ(g[0][0].dest, 1);
  ASSERT_EQ(g[1].size(), 0);

  ASSERT_EQ(field.Dist(0, 0), 0);
  ASSERT_EQ(field.Dist(0, 1), 1);
  ASSERT_TRUE(field.Dist(1, 0) > 100);
}

TEST(map, Serialize) {
  Map field;
  field.Deserialize(sample_json);
  string str = field.SerializeString();
  ASSERT_EQ(str, R"({"mines":[0],"rivers":[{"dest":1,"source":0}],"sites":[{"id":0},{"id":1}]})");
}
