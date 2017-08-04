#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <signal.h>
#include "spawn.h"
#include "strings.h"
#include "map.h"
#include "move.h"
#include "cout.h"
using namespace std;

int SETUP_TIMEOUT_MS = 10000;
int GAMEPLAY_TIMEOUT_MS = 1000;

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

ostream& operator<<(ostream& stream, const Punter& punter) {
  stream << "Punter(" << punter.Id() << ", \"" << punter.Path() << "\")";
  return stream;
}

struct PunterState {
  int n_consecutive_timeout = 0;
  bool is_zombie = false;
  string state;
};

ostream& operator<<(ostream& stream, const PunterState& ps) {
  stream << "PunterState("
    << "n_consecutive_timeout=" << ps.n_consecutive_timeout << ", "
    << "is_zombie=" << ps.is_zombie << ", "
    << "state=\"" << ps.state << "\")";
  return stream;
}

PunterState ParseSetupOutput(const Punter& p, const string& output) {
  picojson::value v;
  string err = picojson::parse(v, output);
  if (!err.empty()) {
    cerr << p << ": Failed to parse setup output: " << output << endl;
    return { 0, true, "" };
  }
  auto o = v.get<picojson::object>();
  string state = o.at("state").get<string>();
  // TODO: validate o.at("ready")
  return { 0, false, state };
}

vector<PunterState> Setup(const vector<Punter>& punters, const Map& map, bool prettify) {

  vector<PunterState> states;
  for (auto& p : punters) {
    picojson::object j;
    j["punter"] = picojson::value((double)p.Id());
    j["punters"] = picojson::value((double)punters.size());
    j["map"] = picojson::value(map.SerializeJson());
    string input = picojson::value(j).serialize(prettify);
    string output;
    auto r = Spawn({p.Path()}, input, SETUP_TIMEOUT_MS, &output);
    switch (r) {
    case SpawnResult::kSuccess:
      states.push_back(ParseSetupOutput(p, output));
      break;
    case SpawnResult::kExecutionFailure:
      states.push_back({ 0, true, "" });
      break;
    case SpawnResult::kTimeout:
      states.push_back({ 1, false, "" });
      break;
    default:
      assert(false);
    }
  }

  return states;
}

int main(int argc, char** argv) {
  if (argc <= 3) {
    fprintf(stderr, "Usage: %s MAP PUNTER_0 PUNTER_1 ...\n", argv[0]);
    exit(2);
  }

  // ignore SIGPIPE
  signal(SIGPIPE, SIG_IGN);

  string map_json = ReadFileOrDie(argv[1]);
  Map map(map_json);

  vector<Punter> punters;
  for (int i = 0; i < argc-2; ++i) {
    punters.push_back(Punter(i, argv[i+2]));
  }

  for (auto& p : punters) {
    cout << p << endl;
  }

  auto states = Setup(punters, map, true);
  cout << "State: " << states << endl;

  return 0;
}
