#include "setting.h"
#include "gtest/gtest.h"
#include <sstream>
#include <iostream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
using namespace std;

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

TEST(setting, Splurges) {
  Setting setting;
  setting.Deserialize(R"({"splurges":true})");
  string str = setting.SerializeString();
  // cout << str << endl;
  ASSERT_TRUE(setting.Splurges());
  ASSERT_EQ(str, R"({"splurges":true})");
}

TEST(setting, Deserialize) {
  Setting setting;
  bool ret = setting.Deserialize("{}");
  ASSERT_TRUE(ret);
  ASSERT_FALSE(setting.Futures());
  string str = setting.SerializeString();
  ASSERT_EQ(str, "{}");
}

TEST(setting, boost_serialize) {
  Setting setting;
  bool ret = setting.Deserialize(sample_json);
  assert(ret);
  std::stringstream ss;
  boost::archive::text_oarchive ar(ss);
  ar << setting;
  boost::archive::text_iarchive ar2(ss);
  Setting setting2;
  ar2 >> setting2;
  ASSERT_TRUE(setting2.Futures());
}
