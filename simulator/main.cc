#include <iostream>
#include <cstdio>
#include <cstring>
#include <getopt.h>
#include <vector>
#include <signal.h>
#include "spawn.h"
#include "strings.h"
#include "map.h"
#include "move.h"
#include "cout.h"
#include "score.h"
#include "scoring.h"
#include "punter.h"
using namespace std;

float timeout_ratio = 1.0;
int HANDSHAKE_TIMEOUT_MS = 1000;
int SETUP_TIMEOUT_MS = 10000;
int GAMEPLAY_TIMEOUT_MS = 1000;
int MAX_TIMEOUT = 10;

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
  process->ReadMessage(HANDSHAKE_TIMEOUT_MS * timeout_ratio, &message);
  process->WriteMessage("{\"you\": \"example\"}", HANDSHAKE_TIMEOUT_MS * timeout_ratio);
}

PunterState SetupPunter(const Punter& p, int n_punters, const Map& map, bool prettify) {
    picojson::object j;
    j["punter"] = picojson::value((double)p.Id());
    j["punters"] = picojson::value((double)n_punters);
    j["map"] = picojson::value(map.SerializeJson());
    string input = picojson::value(j).serialize(prettify);

    auto process = SpawnProcess({p.Path()});
    if (process == nullptr) {
      return { 1, false, "", Move::Pass(p.Id()) };
    }

    Handshake(process.get());
    if (process->WriteMessage(input, SETUP_TIMEOUT_MS * timeout_ratio) != SpawnResult::kSuccess) {
      return { 1, false, "", Move::Pass(p.Id()) };
    }
    process->CloseStdin();
    string output;
    if (process->ReadMessage(SETUP_TIMEOUT_MS * timeout_ratio, &output) != SpawnResult::kSuccess) {
      return { 1, false, "", Move::Pass(p.Id()) };
    }

    return ParseSetupOutput(p, output);
}

vector<PunterState> DoSetup(const vector<Punter>& punters, const Map& map, bool prettify) {
  vector<PunterState> states;
  for (auto& p : punters) {
    states.push_back(SetupPunter(p, punters.size(), map, prettify));
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
    const Punter& punter,
    const Map& map,
    bool prettify,
    /* inout */ vector<PunterState>* punter_states,
    /* inout */ MapState* map_state) {

  PunterState& punter_state = (*punter_states)[punter.Id()];

  if (punter_state.is_zombie) {
    fprintf(stderr, "Punter %d is zombie.\n", punter.Id());
    return;
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

  if (p->WriteMessage(input, GAMEPLAY_TIMEOUT_MS * timeout_ratio) != SpawnResult::kSuccess)
    goto error;
  p->CloseStdin();
  if (p->ReadMessage(GAMEPLAY_TIMEOUT_MS * timeout_ratio, &output) != SpawnResult::kSuccess)
    goto error;
  if (ParseRoundOutput(punter, output, &punter_state.prev_move, &punter_state.state) != kOk)
    goto error;
  if (map_state->ApplyMove(map, punter_state.prev_move) != kOk)
    goto error;
  cout << punter << ": " << punter_state.prev_move << "\n";
  return;

error:
  punter_state.n_timeout++;
  punter_state.prev_move = Move::Pass(punter.Id());
  if (punter_state.n_timeout == MAX_TIMEOUT) {
    punter_state.is_zombie = true;
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
    const Punter& p = punters[round % punters.size()];
    DoRound(p, map, prettify, punter_states, map_state);
  }
}

void DoScoring(
    const vector<Punter>& punters,
    const vector<PunterState>& punter_states,
    const Map& map,
    const MapState& map_state) {

  picojson::array scores;
  for (const auto& punter : punters) {
    int64_t s = ScorePunter(punter.Id(), map, map_state);
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
    Handshake(p.get());
    if (p->WriteMessage(input, GAMEPLAY_TIMEOUT_MS * timeout_ratio) != SpawnResult::kSuccess)
      goto error;
    p->CloseStdin();
    continue;

error:
    /* do nothing */;
  }

  cout << "Final Scores: " << picojson::value(scores).serialize() << endl;
}

void usage(char** argv) {
  fprintf(stderr, "Usage: %s [-t timeout_ratio] MAP PUNTER_0 PUNTER_1 ...\n", argv[0]);
  exit(2);
}

int main(int argc, char** argv) {
  int opt;
  while ((opt = getopt(argc, argv, "ht:")) != -1) {
    switch (opt) {
      case 'h':
        usage(argv);
        break;
      case 't':
        timeout_ratio = atof(optarg);
        break;
      default: /* '?' */
        usage(argv);
    }
  }

  if (argc - optind <= 2) { usage(argv); }

  // ignore SIGPIPE
  signal(SIGPIPE, SIG_IGN);

  string map_json = ReadFileOrDie(argv[optind]);
  Map map(map_json);

  vector<Punter> punters;
  for (int i = 0; i < argc - optind - 1; ++i) {
    punters.push_back(Punter(i, argv[optind + i + 1]));
  }

  for (auto& p : punters) {
    cout << p << endl;
  }

  auto states = DoSetup(punters, map, false);
  cout << "Initial State: " << states << endl;

  MapState map_state(map);
  DoGame(punters, map, false, &states, &map_state);
  cout << "Last State: " << states << endl;
  cout << "Map State: " << map_state << endl;

  DoScoring(punters, states, map, map_state);

  return 0;
}
