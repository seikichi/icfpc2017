#include "spawn.h"
#include "strings.h"
#include <cassert>
#include <condition_variable>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <memory>
using namespace std;


SpawnResult Spawn(
    const vector<string>& command,
    const string& input,
    int timeout_millis,
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
    setpgid(0, 0);

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

    execvp(path.c_str(), &args[0]);

    // There must be an error
    string message = string("Failed to exec child process: ") + JoinString(command, " ");
    perror(message.c_str());
    exit(100);
  }

  // parent
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
  steady_clock::time_point deadline = steady_clock::now() + milliseconds(timeout_millis);

  {
    unique_lock<mutex> lock(mtx);
    while (result == -256) {
      cv.wait_until(lock, deadline);
      if (deadline < steady_clock::now()) {
        fprintf(stderr, "Child process timed out.\n");
        killpg(pid, SIGKILL);
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


std::unique_ptr<Process> SpawnProcess(const std::vector<std::string>& command) {
  assert(!command.empty());

  int pipe_stdin_fds[2];
  if (pipe(pipe_stdin_fds) == -1) {
    perror("pipe");
    return nullptr;
  }
  int pipe_stdout_fds[2];
  if (pipe(pipe_stdout_fds) == -1) {
    perror("pipe");
    return nullptr;
  }

  pid_t pid = fork();
  if (pid == -1) {
    string message = string("Failed to fork child process: ") + JoinString(command, " ");
    perror(message.c_str());
    return nullptr;
  }
  if (pid == 0) {
    // child
    setpgid(0, 0);

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

    execvp(path.c_str(), &args[0]);

    // There must be an error
    string message = string("Failed to exec child process: ") + JoinString(command, " ");
    perror(message.c_str());
    exit(100);
  }

  // parent
  close(pipe_stdin_fds[0]);
  close(pipe_stdout_fds[1]);
  int stdin_fd = pipe_stdin_fds[1];
  int stdout_fd = pipe_stdout_fds[0];

  return make_unique<Process>(pid, stdin_fd, stdout_fd);
}

SpawnResult Process::WriteMessage(const string& json, int timeout_millis) {
  string message = to_string(json.size());
  message += ':';
  message += json;

  condition_variable cv;
  mutex mtx;
  bool done = false;

  thread writer_thread([this, &message, &cv, &mtx, &done]() {
    FILE* f = fdopen(stdin_fd, "wb");
    if (f == nullptr) {
      fprintf(stderr, "Failed to fdopen for write\n");
      close(stdin_fd);
      return;
    }
    fwrite(message.c_str(), 1, message.size(), f);
    fclose(f);
    close(stdin_fd);

    {
      unique_lock<mutex> lock(mtx);
      done = true;
    }
    cv.notify_one();
  });

  using namespace chrono;
  steady_clock::time_point deadline = steady_clock::now() + milliseconds(timeout_millis);

  SpawnResult r = SpawnResult::kSuccess;
  {
    unique_lock<mutex> lock(mtx);
    while (!done) {
      cv.wait_until(lock, deadline);
      if (deadline < steady_clock::now()) {
        Kill();
        fprintf(stderr, "Child process timed out.\n");
        r = SpawnResult::kTimeout;
        break;
      }
    }
  }

  writer_thread.join();
  return r;
}

SpawnResult Process::ReadMessage(int timeout_millis, /* out */ string* message) {
  condition_variable cv;
  mutex mtx;
  bool done = false;

  thread reader_thread([this, message, &cv, &mtx, &done]() {
    int message_length = -1;
    string s = captured;
    for (;;) {
      char buffer[4096];
      ssize_t n = read(stdout_fd, buffer, sizeof(buffer));
      if (n == 0)
        break;
      if (n == -1) {
        perror("Failed to read from child process");
        break;
      }
      s.insert(s.end(), buffer, buffer + n);
      if (message_length == -1) {
        size_t colon_index = s.find(':');
        if (colon_index != string::npos) {
          message_length = stoi(s.substr(0, colon_index));
          s = s.substr(colon_index + 1);
        }
      } else if ((int)s.size() >= message_length) {
        break;
      }
    }


    {
      unique_lock<mutex> lock(mtx);
      if (message_length == -1) {
        fprintf(stderr, "Invalid message: no JSON length prefix\n");
      } else {
        *message = s.substr(0, message_length);
        captured = s.substr(message_length);
      }
      done = true;
    }
    cv.notify_one();
  });

  using namespace chrono;
  steady_clock::time_point deadline = steady_clock::now() + milliseconds(timeout_millis);

  SpawnResult r = SpawnResult::kSuccess;
  {
    unique_lock<mutex> lock(mtx);
    while (!done) {
      cv.wait_until(lock, deadline);
      if (deadline < steady_clock::now()) {
        Kill();
        fprintf(stderr, "Child process timed out.\n");
        r = SpawnResult::kTimeout;
        break;
      }
    }
  }

  reader_thread.join();
  return r;
}

SpawnResult Process::Wait(int timeout_millis) {
  if (pid == -1)
    return SpawnResult::kSuccess;

  condition_variable cv;
  mutex mtx;

  int result = -1;
  bool done = false;

  thread wait_thread([this, &result, &mtx, &cv, &done]() {
    int status;
    int err = waitpid(pid, &status, 0);
    if (err == -1) {
      perror("waitpid");
    }

    {
      unique_lock<mutex> lock(mtx);
      result = WEXITSTATUS(status);
      done = true;
    }
    cv.notify_one();
  });

  using namespace chrono;
  steady_clock::time_point deadline = steady_clock::now() + milliseconds(timeout_millis);

  SpawnResult r = SpawnResult::kSuccess;
  {
    unique_lock<mutex> lock(mtx);
    while (!done) {
      cv.wait_until(lock, deadline);
      if (deadline < steady_clock::now()) {
        Kill();
        fprintf(stderr, "Child process timed out.\n");
        r = SpawnResult::kTimeout;
        break;
      }
    }
  }

  wait_thread.join();

  if (stdin_fd != -1) {
    close(stdin_fd);
    stdin_fd = -1;
  }
  if (stdout_fd != -1) {
    close(stdout_fd);
    stdout_fd = -1;
  }

  pid = -1;
  return r;
}

void Process::Kill() {
  if (pid != -1)
    killpg(pid, SIGKILL);
}
