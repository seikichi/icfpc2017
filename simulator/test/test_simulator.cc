#include "simulator.h"
#include "gtest/gtest.h"

TEST(simulator, add) {
  ASSERT_EQ(add(1, 2), 3);
}
