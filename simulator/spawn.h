#pragma once
#include <vector>
#include <string>

enum class SpawnResult {
  kSuccess = 0,
  kExecutionFailure = 1,
  kTimeout = 2,
};

SpawnResult Spawn(
    const std::vector<std::string>& command,
    const std::string& input,
    int timeout_millis,
    std::string* output);


class Process {
public:


private:
  Process(pid_t pid, int stdin_fd, int stdout_fd):
    pid(pid), stdin_fd(stdin_fd), stdout_fd(stdout_fd) {}

  pid_t pid;
  int stdin_fd;
  int stdout_fd;
};
