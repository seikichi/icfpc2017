#include <iostream>
#include <memory>
#include "protocol.h"
#include "strings.h"
#include "map.h"
#include "scoring.h"

using namespace std;

string MakeState(const Game& game, const MapState& ms) {
  picojson::object o;
  o["game"] = picojson::value(game.SerializeJson());
  o["map_state"] = picojson::value(ms.SerializeJson());
  return picojson::value(o).serialize();
}

void FromState(const string& state, Game* game, MapState* ms) {
  picojson::object o = StringToJson(state);
  game->Deserialize(o["game"].get<picojson::object>());
  ms->Deserialize(o["map_state"].get<picojson::object>());
}

int main(int, char**) {
  auto name = "YAGI";
  auto protocol = make_unique<OfflineClientProtocol>();

  protocol->SetPlayerName(name);
  protocol->Receive();

  if (protocol->Phase() == GamePhase::kSetup) {
    cerr << "Setup" << endl;
    auto game = protocol->Game();
    cerr << game.SerializeString() << endl;

    MapState initial_map_state(game.Map());
    protocol->SetState(MakeState(game, initial_map_state));

  } else if (protocol->Phase() == GamePhase::kGamePlay) {
    cerr << "GamePlay: state = " << protocol->State() << endl;

    Game game;
    MapState map_state(game.Map());
    FromState(protocol->State(), &game, &map_state);

    auto sites = game.Map().Sites();

    for (const Move& m : protocol->OtherMoves()) {
      if (map_state.ApplyMove(game.Map(), m) != kOk) {
        cerr << "Illegal move: " << m << endl;
        continue;
      }
    }

    int64_t max_score = -1;
    Move best_move;

    for (auto& edges : game.Map().Graph()) {
      for (auto& edge : edges) {
        MapState tmp_map_state = map_state;

        auto src = sites[edge.src].original_id;
        auto dest = sites[edge.dest].original_id;
        auto m = Move::Claim(game.PunterID(), src, dest);

        if (tmp_map_state.ApplyMove(game.Map(), m) != kOk)
          continue;

        int64_t score = ScorePunter(game.PunterID(), game.Map(), tmp_map_state);
        if (score > max_score) {
          max_score = score;
          best_move = m;
        }
      }
    }

    protocol->SetPlayerMove(best_move);

    protocol->SetState(MakeState(game, map_state));

  } else if (protocol->Phase() == GamePhase::kScoring) {
    cerr << "Scoring" << endl;
    for (auto score : protocol->Scores()) {
      cerr << "PunterID = " << score.PunterID() << ", Score = " << score.Get() << endl;
    }
  }
  cerr << "Send" << endl;
  protocol->Send();

  return 0;
}
