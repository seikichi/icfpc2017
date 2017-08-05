#include "setting.h"
#include "gtest/gtest.h"

const string sample_json = R"(
{
  "futures": true
}
)";

TEST(setting, declear) {
  Setting setting;
  setting.Clear();
}

TEST(setting, Deseralize) {
  Setting setting;
  bool ret = setting.Deserialize(sample_json);
  ASSERT_TRUE(ret);
  ASSERT_TRUE(setting.Futures());
}

TEST(setting, Serialize) {
  Setting setting;
  setting.Deserialize(sample_json);
  string str = setting.SerializeString();
  // cout << str << endl;
  ASSERT_EQ(str, R"({"futures":true})");
}

TEST(setting, Deserialize) {
  Setting setting;
  bool ret = setting.Deserialize("{}");
  ASSERT_TRUE(ret);
  ASSERT_FALSE(setting.Futures());
  string str = setting.SerializeString();
  ASSERT_EQ(str, "{}");
}
