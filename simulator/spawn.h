#pragma once
#include <vector>
#include <string>
#include <memory>

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
  Process(pid_t pid, int stdin_fd, int stdout_fd):
    pid(pid), stdin_fd(stdin_fd), stdout_fd(stdout_fd) {}
  ~Process() {
    Wait(1000);
  }

  SpawnResult WriteMessage(const std::string& json, int timeout_millis);
  SpawnResult ReadMessage(int timeout_millis, /*out*/ std::string* message);
  SpawnResult Wait(int timeout_millis);
  void Kill();

private:
  pid_t pid;
  int stdin_fd;
  int stdout_fd;
  std::string captured;
};

std::unique_ptr<Process> SpawnProcess(const std::vector<std::string>& command);

