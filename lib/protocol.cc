#include "protocol.h"

#include <cstdlib>
#include <fstream>
#include <memory>
#include <stdio.h>
#include "picojson.h"

#include "strings.h"
using namespace picojson;

const string kLogFileEnvVar = "PUNTER_LOG_FILE";
void WriteSetupLog(const Game& game);
void WriteGamePlayLog(const Move& move, const vector<Move>& other_moves);
void WriteStopLog(const picojson::object& json);

GamePhase OfflineClientProtocol::Receive() {
  phase = GamePhase::kHandshake;
  SendName();
  ReceiveName();
  phase = GamePhase::kFinishHandshake;
  string str = ReceiveString();
  const picojson::object json = StringToJson(str);
  if (json.count("punter")) {
    phase = GamePhase::kSetup;
    game.Deserialize(json);
  } else if (json.count("move")) {
    phase = GamePhase::kGamePlay;
    const auto &l_moves = json.at("move").get<picojson::object>().at("moves").get<picojson::array>();
    for (const value &v : l_moves) {
      const auto &o = v.get<object>();
      other_moves.emplace_back(Move::Deserialize(o));
    }
    prev_state = json.at("state").get<string>();
  } else if (json.count("stop")) {
    phase = GamePhase::kScoring;
    const auto &l_stop = json.at("stop").get<picojson::object>();
    const auto &l_moves = l_stop.at("moves").get<picojson::array>();
    for (const value &v : l_moves) {
      const auto &o = v.get<object>();
      other_moves.emplace_back(Move::Deserialize(o));
    }
    const auto &l_scores = l_stop.at("scores").get<picojson::array>();
    for (const value &v : l_scores) {
      const auto &o = v.get<object>();
      scores.emplace_back(Score(o));
    }

    WriteStopLog(json);
    // NOTE: lamduct (official server?) does not return `state` property
    // prev_state = json.at("state").get<string>();
  } else if (json.count("timeot")) {
    phase = GamePhase::kTimeOut;
    timeout = json.at("timeout").get<double>();
  } else {
    phase = GamePhase::kUnknown;
    fprintf(stderr, "Unknown Json\n");
  }
  return phase;
}

void OfflineClientProtocol::Send() {
  if (phase == GamePhase::kHandshake) {
    assert(false);
    // Receiveの方で勝手にやる
    SendName();
  } else if (phase == GamePhase::kSetup) {
    assert(next_state != "");
    picojson::object l_ready;
    l_ready["ready"] = picojson::value((double)game.PunterID());
    if (game.Setting().Futures()) {
      picojson::array l_futures = picojson::array();
      for (const auto &future : player_futures) {
        l_futures.emplace_back(future.SerializeJson());
      }
      l_ready["futures"] = picojson::value(l_futures);
    }
    l_ready["state"] = picojson::value(next_state);

    WriteSetupLog(game);
    SendString(picojson::value(l_ready).serialize());
  } else if (phase == GamePhase::kGamePlay) {
    assert(next_state != "");
    assert(player_move.PunterID() != -1);
    picojson::object l_move = player_move.SerializeJson();
    l_move["state"] = picojson::value(next_state);

    WriteGamePlayLog(player_move, other_moves);
    SendString(picojson::value(l_move).serialize());
  } else {
    fprintf(stderr, "This Phase can't send\n");
  }
}

string OfflineClientProtocol::ReceiveString() {
  int json_size;
  char c;
  int v = scanf("%d%c", &json_size, &c);
  unique_ptr<char[]> buffer = make_unique<char[]>(json_size + 10);
  assert(v == 2);
  int size = fread(buffer.get(), sizeof(char), json_size, stdin);
  buffer[size] = 0;
  string ret = string(buffer.get());
  return ret;
}

void OfflineClientProtocol::SendString(const string &str) {
  printf("%lu:%s", str.size(), str.c_str());
  fflush(stdout);
}

void ClientProtocol::ReceiveName() {
  string str = ReceiveString();
  // picojson::object json = StringToJson(str);
  // string you = json.at("you").get<string>();
  // assert(you == player_name);
}

void ClientProtocol::SendName() {
  picojson::object l_name;
  l_name["me"] = picojson::value(player_name);
  string str = picojson::value(l_name).serialize();
  SendString(str);
}

bool IsLogMode() {
  auto env = getenv(kLogFileEnvVar.c_str());
  return env != nullptr;
}

bool IsSingleLogMode() {
  string env = getenv(kLogFileEnvVar.c_str());
  return env.find("#{punter_id}", 0) == string::npos;
}

ofstream OpenLogStream(int punter_id, ios_base::openmode mode) {
  auto env = getenv(kLogFileEnvVar.c_str());
  assert(env != nullptr);
  assert(punter_id != -1);

  auto filename = ReplaceString(env, "#{punter_id}", to_string(punter_id));
  std::ofstream stream;
  stream.open(filename, mode);
  assert(!stream.fail());

  return stream;
}

void WriteSetupLog(const Game& game) {
  if (IsLogMode()) {
    auto map = game.Map();
    int num_edges = 0;
    for (auto edges: map.Graph()) {
      num_edges += edges.size();
    }
    num_edges /= 2;

    ofstream stream = OpenLogStream(game.PunterID(), ios_base::out);
    stream <<
      "{" <<
      "\"punter\":" << game.PunterID() << ", " <<
      "\"punters\":" << game.PunterNum() << ", " <<
      "\"num_nodes\":" << map.Sites().size() << ", " <<
      "\"num_edges\":" << num_edges <<
      "}" << endl;
  }
}

void WriteGamePlayLog(const Move& move, const vector<Move>& other_moves) {
  if (IsLogMode()) {
    ofstream stream = OpenLogStream(move.PunterID(), ios_base::app);
    for (int i = 0; i < (int)other_moves.size(); i++) {
      auto m = other_moves[(move.PunterID() + i) % other_moves.size()];
      stream << picojson::value(m.SerializeJson()).serialize() << endl;
    }
  }
}

void WriteStopLog(const picojson::object& json) {
  if (IsLogMode() && IsSingleLogMode()) {
    ofstream stream = OpenLogStream(0, ios_base::app);
    const auto &stop = json.at("stop");
    stream << "{\"stop\": " << stop.serialize() << "}" << endl;
  }
}
