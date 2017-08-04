#include <iostream>
#include <cassert>
#include <picojson.h>
#include <thread>
#include <condition_variable>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
using namespace std;

enum class SpawnResult {
  kSuccess = 0,
  kExecutionFailure = 1,
  kTimeout = 2,
};

std::string JoinString(const vector<string>& strings, const string& separator) {
  string message = "";
  for (int i = 0; i < (int)strings.size(); ++i) {
    if (i != 0)
      message += separator;
    message += strings[i];
  }
  return message;
}

SpawnResult Spawn(
    const vector<string>& command,
    const string& input,
    int timeoutMillis,
    string* output) {

  assert(!command.empty());

  int pipe_stdin_fds[2];
  if (pipe(pipe_stdin_fds) == -1) {
    perror("pipe");
    return SpawnResult::kExecutionFailure;
  }
  int pipe_stdout_fds[2];
  if (pipe(pipe_stdout_fds) == -1) {
    perror("pipe");
    return SpawnResult::kExecutionFailure;
  }

  pid_t pid = fork();
  if (pid == -1) {
    string message = string("Failed to fork child process: ") + JoinString(command, " ");
    perror(message.c_str());
    return SpawnResult::kExecutionFailure;
  }
  if (pid == 0) {
    // child
    if (dup2(pipe_stdin_fds[0], STDIN_FILENO) == -1) {
      perror("dup2");
      exit(100);
    }
    close(pipe_stdin_fds[0]);
    close(pipe_stdin_fds[1]);

    if (dup2(pipe_stdout_fds[1], STDOUT_FILENO) == -1) {
      perror("dup2");
      exit(100);
    }
    close(pipe_stdout_fds[0]);
    close(pipe_stdout_fds[1]);

    const std::string& path = command[0];

    vector<char *> args(command.size() + 1);
    for (int i = 0; i < (int)command.size(); ++i) {
      args[i] = const_cast<char*>(command[i].c_str());
    }
    args[command.size()] = nullptr;

    execv(path.c_str(), &args[0]);

    // There must be an error
    string message = string("Failed to exec child process: ") + JoinString(command, " ");
    perror(message.c_str());
    exit(100);
  }

  // master
  close(pipe_stdin_fds[0]);
  close(pipe_stdout_fds[1]);
  int stdin_fd = pipe_stdin_fds[1];
  int stdout_fd = pipe_stdout_fds[0];

  condition_variable cv;
  mutex mtx;
  int result = -256;

  thread wait_thread([pid, &command, &result, &mtx, &cv]() {
    int status;
    int err = waitpid(pid, &status, 0);
    if (err == -1) {
      string message = string("Failed to wait child process: ") + JoinString(command, " ");
      perror(message.c_str());
      result = -42;
      return;
    }

    {
      unique_lock<mutex> lock(mtx);
      result = WEXITSTATUS(status);
    }
    cv.notify_one();
  });

  thread input_thread([stdin_fd, &input]() {
    FILE* f = fdopen(stdin_fd, "wb");
    if (f == nullptr) {
      fprintf(stderr, "Failed to fdopen for write\n");
      close(stdin_fd);
      return;
    }
    fwrite(input.c_str(), input.size(), 1, f);
    fclose(f);
    close(stdin_fd);
  });

  thread capture_thread([stdout_fd, output]() {
    string captured;
    for (;;) {
      char buffer[4096];
      ssize_t n = read(stdout_fd, buffer, sizeof(buffer));
      if (n == 0)
        break;
      if (n == -1) {
        perror("Failed to read from child process");
        break;
      }
      captured.insert(captured.end(), buffer, buffer + n);
    }
    close(stdout_fd);
    *output = captured;
  });

  SpawnResult err = SpawnResult::kSuccess;

  using namespace chrono;
  steady_clock::time_point deadline = steady_clock::now() + milliseconds(timeoutMillis);

  {
    unique_lock<mutex> lock(mtx);
    while (result == -256) {
      cv.wait_until(lock, deadline);
      if (deadline < steady_clock::now()) {
        fprintf(stderr, "Child process timed out.\n");
        kill(pid, SIGKILL);
        err = SpawnResult::kTimeout;
        break;
      }
    }
  }

  if (result != 0 && err == SpawnResult::kSuccess) {
    fprintf(stderr, "Child process returned an error: status=%d\n", result);
    err = SpawnResult::kExecutionFailure;
  }

  wait_thread.join();
  input_thread.join();
  capture_thread.join();
  return err;
}

int main() {
  /*
  std::string json = "[ \"hello JSON\" ]";
  picojson::value v;
  std::string err = picojson::parse(v, json);
  if (! err.empty()) {
    std::cerr << err << std::endl;
  } else {
    std::cerr << "ok" << std::endl;
  }
  */
  vector<string> command = { "/bin/sleep", "1" };
  string output;
  auto r = Spawn(command, "", 1000, &output);
  cout << "Output: " << output << endl;
  cout << "Result: " << (int)r << endl;
  return 0;
}
