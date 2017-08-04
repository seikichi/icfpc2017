#include "map.h"
#include "gtest/gtest.h"

TEST(map, declear) {
  Map field;
  field.Clear();
}

TEST(map, Init) {
  string json = R"(
{
  "sites": [
    {"id":1},
    {"id":2}
  ],
  "rivers": [
    {"source":1, "target":2}
  ],
  "mines": [
    1
  ]
}
)";
  Map field;
  bool ret = field.Init(json);
  ASSERT_TRUE(ret);
}
