#include "strings.h"
#include "gtest/gtest.h"

TEST(strings, ReplaceString) {
  auto actual = ReplaceString("abc def abc def", "abc", "hij");
  ASSERT_EQ(actual, "hij def hij def");
}
