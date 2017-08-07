#include "map.h"
#include "gtest/gtest.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/serialization.hpp>

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
  ASSERT_EQ(g[0][0].id, 0);
  ASSERT_EQ(g[1].size(), 1);
  ASSERT_EQ(g[1][0].dest, 0);
  ASSERT_EQ(g[1][0].id, 0);

  ASSERT_EQ(field.Dist(0, 0), 0);
  ASSERT_EQ(field.Dist(0, 1), 1);
  ASSERT_EQ(field.Dist(1, 0), 1);
}

TEST(map, Serialize) {
  Map field;
  field.Deserialize(sample_json);
  string str = field.SerializeString();
  // cout << str << endl;
  ASSERT_EQ(str, R"({"mines":[0],"rivers":[{"source":0,"target":1}],"sites":[{"id":0},{"id":1}]})");
}

TEST(field, boost_serialize) {
  Map field;
  bool ret = field.Deserialize(sample_json);
  assert(ret);

  std::stringstream ss;
  boost::archive::text_oarchive ar(ss);
  ar << field;
  // cout << ss.str() << endl;

  ss.clear();
  boost::archive::text_iarchive ar2(ss);
  Map field2;
  ar2 >> field2;
  field2.InitDists();

  const vector<Site> &sites = field2.Sites();
  const Graph &g = field2.Graph();

  ASSERT_EQ(sites.size(), 2);
  ASSERT_TRUE(sites[0].is_mine);
  ASSERT_FALSE(sites[1].is_mine);

  ASSERT_EQ(g.size(), 2);
  ASSERT_EQ(g[0].size(), 1);
  ASSERT_EQ(g[0][0].dest, 1);
  ASSERT_EQ(g[0][0].id, 0);
  ASSERT_EQ(g[1].size(), 1);
  ASSERT_EQ(g[1][0].dest, 0);
  ASSERT_EQ(g[1][0].id, 0);

  ASSERT_EQ(field2.Dist(0, 0), 0);
  ASSERT_EQ(field2.Dist(0, 1), 1);
  ASSERT_EQ(field2.Dist(1, 0), 1);
}
