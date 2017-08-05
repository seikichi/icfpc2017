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
#include "score.h"
using namespace std;

int HANDSHAKE_TIMEOUT_MS = 1000;
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
  Move prev_move = Move::Pass(-1);
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
    return { 0, true, "",  Move::Pass(p.Id()) };
  }
  auto o = v.get<picojson::object>();
  string state = o.at("state").get<string>();
  // TODO: validate o.at("ready")
  return { 0, false, state, Move::Pass(p.Id()) };
}

void Handshake(Process* process) {
  string message;
  process->ReadMessage(HANDSHAKE_TIMEOUT_MS, &message);
  process->WriteMessage("{\"you\": \"example\"}", HANDSHAKE_TIMEOUT_MS);
}

vector<PunterState> Setup(const vector<Punter>& punters, const Map& map, bool prettify) {

  vector<PunterState> states;
  for (auto& p : punters) {
    picojson::object j;
    j["punter"] = picojson::value((double)p.Id());
    j["punters"] = picojson::value((double)punters.size());
    j["map"] = picojson::value(map.SerializeJson());
    string input = picojson::value(j).serialize(prettify);

    auto process = SpawnProcess({p.Path()});
    if (process == nullptr) {
      states.push_back({ 1, false, "", Move::Pass(p.Id()) });
      continue;
    }

    Handshake(process.get());
    if (process->WriteMessage(input, SETUP_TIMEOUT_MS) != SpawnResult::kSuccess) {
      states.push_back({ 1, false, "", Move::Pass(p.Id()) });
      continue;
    }
    string output;
    if (process->ReadMessage(SETUP_TIMEOUT_MS, &output) != SpawnResult::kSuccess) {
      states.push_back({ 1, false, "", Move::Pass(p.Id()) });
      continue;
    }

    states.push_back(ParseSetupOutput(p, output));
  }

  return states;
}

Error ParseRoundOutput(
    const Punter& punter,
    const string& output,
    /* out */ Move* move,
    /* out */ string* state) {

  picojson::value v;
  string err = picojson::parse(v, output);
  if (!err.empty()) {
    fprintf(stderr, "Punter %d returned invalid JSON: %s: %s\n", punter.Id(), err.c_str(), output.c_str());
    return kBad;
  }

  auto o = v.get<picojson::object>();
  *move = Move::Deserialize(o);
  *state = o.at("state").get<string>();
  return kOk;
}

picojson::array MakeMovesJson(const vector<PunterState>& punter_states) {
  picojson::array moves;
  for (int i = 0; i < (int)punter_states.size(); ++i) {
    auto v = picojson::value(punter_states[i].prev_move.SerializeJson());
    moves.push_back(v);
  }
  return moves;
}

void DoRound(
    const vector<Punter>& punters,
    const Map& map,
    bool prettify,
    /* inout */ vector<PunterState>* punter_states,
    /* inout */ MapState* map_state) {

  for (int i = 0; i < (int)punters.size(); ++i) {
    const Punter& punter = punters[i];
    PunterState& punter_state = (*punter_states)[i];

    if (punter_state.is_zombie) {
      fprintf(stderr, "Punter %d is zombie.\n", punter.Id());
      continue;
    }

    picojson::object j_moves, j_move;
    auto a_moves = MakeMovesJson(*punter_states);
    j_moves["moves"] = picojson::value(a_moves);
    j_move["move"] = picojson::value(j_moves);

    picojson::object j = j_move;
    j["state"] = picojson::value(punter_state.state);
    string input = picojson::value(j).serialize(prettify);
    string output;

    auto p = SpawnProcess({punter.Path()});
    if (p == nullptr)
      goto error;

    Handshake(p.get());

    if (p->WriteMessage(input, GAMEPLAY_TIMEOUT_MS) != SpawnResult::kSuccess)
      goto error;
    if (p->ReadMessage(GAMEPLAY_TIMEOUT_MS, &output) != SpawnResult::kSuccess)
      goto error;
    if (ParseRoundOutput(punter, output, &punter_state.prev_move, &punter_state.state) != kOk)
      goto error;
    if (map_state->ApplyMove(map, punter.Id(), punter_state.prev_move) != kOk)
      goto error;
    punter_state.n_consecutive_timeout = 0;
    cout << punter << ": " << punter_state.prev_move << "\n";
    continue;

error:
    punter_state.n_consecutive_timeout++;
    punter_state.prev_move = Move::Pass(punter.Id());
    if (punter_state.n_consecutive_timeout == MAX_CONSECUTIVE_TIMEOUT) {
      punter_state.is_zombie = true;
    }
  }
}

void DoGame(
    const vector<Punter>& punters,
    const Map& map,
    bool prettify,
    /* inout */ vector<PunterState>* punter_states,
    /* inout */ MapState* map_state) {

  int n_rounds = 0;
  for (const auto& es : map.Graph()) {
    n_rounds += (int)es.size();
  }
  assert(n_rounds % 2 == 0);
  n_rounds /= 2;

  for (int round = 0; round < n_rounds; ++round) {
    cout << "[[[[[ Round #" << round << " ]]]]]\n";
    DoRound(punters, map, prettify, punter_states, map_state);
  }
}

inline int64_t Square(int64_t x) { return x*x; }

int64_t Dfs(
    int site,
    int mine,
    int punter_id,
    const Map& map,
    const MapState& map_state,
    /* inout */ vector<bool>* visited) {

  (*visited)[site] = true;

  int64_t sum = Square(map.Dist(mine, site));
  for (const auto& e : map.Graph()[site]) {
    if ((*visited)[e.dest])
      continue;
    if (map_state.Claimer(e.id) != punter_id)
      continue;
    sum += Dfs(e.dest, mine, punter_id, map, map_state, visited);
  }
  return sum;
}

int64_t ScoreMine(
    int mine,
    int punter_id,
    const Map& map,
    const MapState& map_state) {

  vector<bool> visited(map.Size());
  return Dfs(mine, mine, punter_id, map, map_state, &visited);
}

int64_t ScorePunter(
    const Punter& punter,
    const Map& map,
    const MapState& map_state) {

  int64_t score = 0;
  for (const auto& site : map.Sites()) {
    if (!site.is_mine)
      continue;
    score += ScoreMine(site.id, punter.Id(), map, map_state);
  }
  return score;
}

void DoScoring(
    const vector<Punter>& punters,
    const vector<PunterState>& punter_states,
    const Map& map,
    const MapState& map_state) {

  picojson::array scores;
  for (const auto& punter : punters) {
    int64_t s = ScorePunter(punter, map, map_state);
    Score score(punter.Id(), s);
    scores.push_back(picojson::value(score.SerializeJson()));
  }

  for (int i = 0; i < (int)punters.size(); ++i) {
    const Punter& punter = punters[i];
    const PunterState state = punter_states[i];

    picojson::object o_stop;
    o_stop["moves"] = picojson::value(MakeMovesJson(punter_states));
    o_stop["scores"] = picojson::value(scores);

    picojson::object j;
    j["stop"] = picojson::value(o_stop);
    j["state"] = picojson::value(state.state);

    string input = picojson::value(j).serialize();
    auto p = SpawnProcess({punter.Path()});
    if (!p)
      goto error;
    if (p->WriteMessage(input, GAMEPLAY_TIMEOUT_MS) != SpawnResult::kSuccess)
      goto error;
    continue;

error:
    /* do nothing */;
  }
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

  MapState map_state(map);
  DoGame(punters, map, false, &states, &map_state);
  cout << "Last State: " << states << endl;
  cout << "Map State: " << map_state << endl;

  DoScoring(punters, states, map, map_state);

  return 0;
}
