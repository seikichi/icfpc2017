#include "protocol.h"
#include <stdio.h>
#include "picojson.h"

#include "strings.h"
using namespace picojson;

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
    prev_state = json.at("state").get<string>();
  } else if (json.count("timeot")) {
    phase = GamePhase::kTimeOut;
    timeout = json.at("timeout").get<double>();
  } else {
    phase = GamePhase::kUnknown;
    fprintf(stderr, "Unknown Json");
  }
  return phase;
}

void OfflineClientProtocol::Send() {
  if (phase == GamePhase::kHandshake) {
    SendName();
  } else if (phase == GamePhase::kSetup) {
    assert(next_state != "");
    picojson::object l_ready;
    l_ready["ready"] = picojson::value((double)game.PunterID());
    if (game.Setting().Futures()) {
      assert(false);
      // TODO implement
      // l_read["futures"] = 
    }
    l_ready["state"] = picojson::value(next_state);
    SendString(picojson::value(l_ready).serialize());
  } else if (phase == GamePhase::kGamePlay) {
    assert(next_state != "");
    picojson::object l_move;
    l_move["move"] = picojson::value(player_move.SerializeJson());
    l_move["state"] = picojson::value(next_state);
    SendString(picojson::value(l_move).serialize());
  } else {
    fprintf(stderr, "This Phase can't send");
  }
}

string OfflineClientProtocol::ReceiveString() {
  char buffer[10000];
  int json_size;
  char c;
  scanf("%d%c", &json_size, &c);
  int size = fread(buffer, sizeof(char), json_size, stdin);
  buffer[size] = 0;
  return string(buffer);
}

void OfflineClientProtocol::SendString(const string &str) {
  printf("%s\n", str.c_str());
}

void ClientProtocol::ReceiveName() {
  string str = ReceiveString();
  picojson::object json = StringToJson(str);
  string me = json.at("me").get<string>();
  assert(me == player_name);
}

void ClientProtocol::SendName() {
  picojson::object l_name;
  l_name["you"] = picojson::value(player_name);
  string str = picojson::value(l_name).serialize();
  SendString(str);
}
