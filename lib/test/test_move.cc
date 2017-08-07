#include "move.h"
#include "gtest/gtest.h"
using namespace std;

const string splurge_json = R"(
{
  "splurge": {
    "punter": 1,
    "route": [ 1, 2, 3, 4 ]
  }
}
)";

TEST(move, splurge) {
  Move move;
  move = Move::Deserialize(splurge_json);
  std::vector<int> route = move.Route();
  ASSERT_EQ(move.PunterID(), 1);
  ASSERT_EQ(route.size(), 4);
  ASSERT_EQ(route[0], 1);
  ASSERT_EQ(route[3], 4);
  string str = move.SerializeString();
  // cout << str << endl;
  ASSERT_EQ(str, R"({"splurge":{"punter":1,"route":[1,2,3,4]}})");
}
