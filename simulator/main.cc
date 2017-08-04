#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include "spawn.h"
#include "strings.h"
#include "map.h"
#include "move.h"
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
  const int id;
  const string path;
};

struct PuterState {
  int n_consecutive_timeout = 0;
  bool is_zombie = false;
};

ostream& operator<<(ostream& stream, const Punter& punter) {
  stream << "Punter(" << punter.Id() << ", \"" << punter.Path() << "\")";
  return stream;
}

int main(int argc, char** argv) {
  if (argc <= 4) {
    fprintf(stderr, "Usage: %s MAP PUNTER_0 PUNTER_1 ...\n", argv[0]);
    exit(2);
  }

  vector<Punter> punters;
  for (int i = 0; i < argc-2; ++i) {
    punters.push_back(Punter(i, argv[i+2]));
  }

  for (auto& p : punters) {
    cout << p << endl;
  }

  return 0;
}
