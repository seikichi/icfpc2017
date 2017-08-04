#include "cout.h"
#include "gtest/gtest.h"
#include <string>

TEST(cout, vector) {
  vector<int> vs = { 1, 2, 3 };
  testing::internal::CaptureStdout();
  cout << vs;
  ASSERT_STREQ("[ 1, 2, 3 ]", testing::internal::GetCapturedStdout().c_str());
}
TEST(cout, deque) {
  deque<int> vs = { 1, 2, 3 };
  testing::internal::CaptureStdout();
  cout << vs;
  ASSERT_STREQ("[ 1, 2, 3 ]", testing::internal::GetCapturedStdout().c_str());
}

TEST(cout, map) {
  map<string, int> vs = { {"A", 1 }, {"B", 2 }};
  testing::internal::CaptureStdout();
  cout << vs;
  ASSERT_STREQ("{\n A : 1,\n B : 2\n}", testing::internal::GetCapturedStdout().c_str());
}
TEST(cout, set) {
  set<int> vs = { 1, 2, 3 };
  testing::internal::CaptureStdout();
  cout << vs;
  ASSERT_STREQ("{ 1, 2, 3 }", testing::internal::GetCapturedStdout().c_str());
}
TEST(cout, pair) {
  pair<string, int> vs = { "A", 1 };
  testing::internal::CaptureStdout();
  cout << vs;
  ASSERT_STREQ("( A, 1 )", testing::internal::GetCapturedStdout().c_str());
}
