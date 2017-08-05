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

TEST(SpawnProcess, Basic) {
  auto p = SpawnProcess({"cat"});

  auto r1 = p->WriteMessage("{\"basic\": 42}", 1000);
  ASSERT_EQ(r1, SpawnResult::kSuccess);

  string message;
  auto r2 = p->ReadMessage(1000, &message);
  ASSERT_EQ(r2, SpawnResult::kSuccess);
  ASSERT_EQ(message, "{\"basic\": 42}");
}

TEST(SpawnProcess, ReadTimeout) {
  auto p = SpawnProcess({"sleep", "1000"});

  string message;
  auto r2 = p->ReadMessage(100, &message);
  ASSERT_EQ(r2, SpawnResult::kTimeout);
}

TEST(SpawnProcess, Large) {
  auto p = SpawnProcess({"./test/mysponge"});

  string input(32*1024*1024, 'x');
  auto r1 = p->WriteMessage(input, 1000);
  ASSERT_EQ(r1, SpawnResult::kSuccess);

  p->CloseStdin();

  string output;
  auto r2 = p->ReadMessage(1000, &output);
  ASSERT_EQ(r2, SpawnResult::kSuccess);
  ASSERT_EQ(output, input);
}

