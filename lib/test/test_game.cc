#include "game.h"
#include "gtest/gtest.h"

const string sample_json = R"(
{
  "punter": 1,
  "punters": 2,
  "map": {
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
}
)";

TEST(game, declear) {
  Game game;
  game.Clear();
}

TEST(game, Deseralize) {
  Game game;
  bool ret = game.Deserialize(sample_json);
  ASSERT_TRUE(ret);
  ASSERT_EQ(game.PunterID(), 1);
  ASSERT_EQ(game.PunterNum(), 2);
  const Map &map = game.Map();
  ASSERT_TRUE(map.Sites()[0].is_mine);
}

TEST(game, Serialize) {
  Game game;
  game.Deserialize(sample_json);
  string str = game.SerializeString();
  // cout << str << endl;
  ASSERT_EQ(str, R"({"map":{"mines":[0],"rivers":[{"dest":1,"source":0}],"sites":[{"id":0},{"id":1}]},"punter":1,"punters":2})");
}
