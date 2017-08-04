#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include "spawn.h"
#include "strings.h"
using namespace std;

class Punter {
public:
  Punter(int id, const string& path):
    id(id), path(path) {}

  int Id() const {
    return id;
  }

  const string& Path() const {
    return path;
  }

private:
  int id;
  string path;
};

ostream& operator<<(ostream& stream, const Punter& punter) {
  stream << "Punter(" << punter.Id() << ", \"" << punter.Path() << "\")";
  return stream;
}

int main(int argc, char** argv) {
  if (argc <= 1) {
    fprintf(stderr, "Usage: %s PUNTER_0 PUNTER_1 ...\n", argv[0]);
    exit(2);
  }

  vector<Punter> punters;
  for (int i = 0; i < argc - 1; ++i) {
    punters.push_back(Punter(i, argv[i + 1]));
  }

  for (auto& p : punters) {
    cout << p << endl;
  }

  return 0;
}
