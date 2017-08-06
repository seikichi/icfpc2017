#include <iostream>
#include <memory>
#include "protocol.h"
#include "strings.h"
#include "map.h"
#include "scoring.h"
#include "ai_runner.h"

using namespace std;

Move AnyMove(const Game& game, const MapState& map_state) {
  auto& sites = game.Map().Sites();
  int punter_id = game.PunterID();
  for (auto& edges : game.Map().Graph()) {
    for (auto& edge : edges) {
      if (map_state.Claimer(edge.id) != -1) {
        continue;
      }
      auto src = sites[edge.src].original_id;
      auto dest = sites[edge.dest].original_id;
      return Move::Claim(punter_id, src, dest);
    }
  }
  return Move::Pass(punter_id);
}

Move DecideByGreedy(const Game& game, const MapState& map_state, int /*my_rounds*/) {
  auto& sites = game.Map().Sites();
  int punter_id = game.PunterID();

  int64_t max_score = -1;
  Move best_move = Move::Pass(punter_id);

  vector<bool> connected(game.Map().Sites().size());

  for (auto& edges : game.Map().Graph()) {
    for (auto& edge : edges) {
      if (map_state.Claimer(edge.id) == punter_id) {
        connected[edge.src] = true;
        connected[edge.dest] = true;
      }
    }
  }

  for (auto& edges : game.Map().Graph()) {
    for (auto& edge : edges) {
      if (map_state.Claimer(edge.id) != -1) {
        continue;
      }
      if (!game.Map().Sites()[edge.src].is_mine &&
          !game.Map().Sites()[edge.dest].is_mine &&
          !connected[edge.src] &&
          !connected[edge.dest]) {
        continue;
      }

      auto src = sites[edge.src].original_id;
      auto dest = sites[edge.dest].original_id;
      auto m = Move::Claim(punter_id, src, dest);

      MapState tmp_map_state = map_state;
      if (tmp_map_state.ApplyMove(game.Map(), m) != kOk)
        continue;

      int64_t score = ScorePunter(game.PunterID(), game.Map(), tmp_map_state);
      if (score > max_score) {
        max_score = score;
        best_move = m;
      }
    }
  }

  if (max_score == -1) {
    return AnyMove(game, map_state);
  }

  return best_move;
}

int main(int, char**) {
  auto name = "YAGI";
  auto protocol = make_unique<OfflineClientProtocol>();

  protocol->SetPlayerName(name);
  protocol->Receive();

  if (protocol->Phase() == GamePhase::kSetup) {
    DoSetup(protocol.get());
  } else if (protocol->Phase() == GamePhase::kGamePlay) {
    cerr << "GamePlay: state = " << protocol->State() << endl;
    RunAiWithTimeoutAndDie(protocol.get(), DecideByGreedy, 900);
  } else if (protocol->Phase() == GamePhase::kScoring) {
    DoScoring(protocol.get());
  }

  return 0;
}
