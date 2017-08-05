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
int MAX_CONSECUTIVE_TIMEOUT = 10;

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
  Move prev_move = Move::Pass();
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
    return { 0, true, "",  Move::Pass() };
  }
  auto o = v.get<picojson::object>();
  string state = o.at("state").get<string>();
  // TODO: validate o.at("ready")
  return { 0, false, state, Move::Pass() };
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
    if (r == SpawnResult::kSuccess) {
      states.push_back(ParseSetupOutput(p, output));
    } else if (r == SpawnResult::kExecutionFailure || r == SpawnResult::kTimeout) {
      states.push_back({ 1, false, "", Move::Pass() });
    } else {
      assert(false);
    }
  }

  return states;
}

pair<Move, string> ParseRoundOutput(const Punter& punter, const string& output) {
  picojson::value v;
  string err = picojson::parse(v, output);
  if (!err.empty()) {
    fprintf(stderr, "Punter %d returned invalid JSON: %s: %s\n", punter.Id(), err.c_str(), output.c_str());
    return make_pair(Move::Pass(), "");
  }

  auto o = v.get<picojson::object>();
  return make_pair(Move::Deserialize(o), o.at("state").get<string>());
}

vector<PunterState> DoRound(
    const vector<Punter>& punters,
    vector<PunterState> states,
    const Map&,
    bool prettify) {

  picojson::array moves;
  for (int i = 0; i < (int)states.size(); ++i) {
    auto v = picojson::value(states[i].prev_move.SerializeJson(i));
    moves.push_back(v);
  }

  picojson::object j_moves, j_move;
  j_moves["moves"] = picojson::value(moves);
  j_move["move"] = picojson::value(j_moves);

  for (int i = 0; i < (int)punters.size(); ++i) {
    const Punter& punter = punters[i];
    PunterState& state = states[i];

    if (state.is_zombie)
      continue;

    picojson::object j = j_move;
    j["state"] = picojson::value(state.state);
    string input = picojson::value(j).serialize(prettify);
    string output;

    auto r = Spawn({punter.Path()}, input, GAMEPLAY_TIMEOUT_MS, &output);
    if (r == SpawnResult::kSuccess) {
      auto p = ParseRoundOutput(punter, output);
      state.prev_move = move(p.first);
      state.state = move(p.second);
      state.n_consecutive_timeout = 0;
      cout << punter << ": " << state.prev_move << "\n";
    } else if (r == SpawnResult::kExecutionFailure || r == SpawnResult::kTimeout) {
      state.n_consecutive_timeout++;
      state.prev_move = Move::Pass();
      if (state.n_consecutive_timeout == MAX_CONSECUTIVE_TIMEOUT) {
        state.is_zombie = true;
      }
    } else {
      assert(false);
    }
  }

  return states;
}

vector<PunterState> DoGame(
    const vector<Punter>& punters,
    vector<PunterState> states,
    const Map& map,
    bool prettify) {

  int n_rounds = map.Graph().size();
  for (int round = 0; round < n_rounds; ++round) {
    cout << "[[[[[ Round #" << round << " ]]]]]\n";
    states = DoRound(punters, move(states), map, prettify);
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

  auto states = Setup(punters, map, false);
  cout << "Initial State: " << states << endl;

  states = DoGame(punters, states, map, false);
  cout << "Last State: " << states << endl;

  return 0;
}
