#include "spawn.h"
#include "gtest/gtest.h"
using namespace std;

TEST(spawn, basic) {
  string output;
  auto r = Spawn({"cat"},
                 "Hello, World!!",
                 1000,
                 &output);
  ASSERT_EQ(r, SpawnResult::kSuccess);
  ASSERT_EQ(output, "Hello, World!!");
}

TEST(spawn, timeout) {
  string output;
  auto r = Spawn({"/bin/sleep", "1000"}, "", 100, &output);
  ASSERT_EQ(r, SpawnResult::kTimeout);
}

TEST(spawn, failure) {
  string output;
  auto r = Spawn({"/bin/false"}, "", 1000, &output);
  ASSERT_EQ(r, SpawnResult::kExecutionFailure);
}

TEST(spawn, notfound) {
  string output;
  auto r = Spawn({"/dummy"}, "", 1000, &output);
  ASSERT_EQ(r, SpawnResult::kExecutionFailure);
}

TEST(spawn, large) {
  string input(32*1024*1024, 'x');
  string output;
  auto r = Spawn({"cat"}, input, 1000, &output);
  ASSERT_EQ(r, SpawnResult::kSuccess);
  ASSERT_EQ(output, input);
}
