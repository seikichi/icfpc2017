#include <iostream>
#include <memory>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include "protocol.h"
#include "strings.h"
#include "map.h"
#include "scoring.h"

using namespace std;

string MakeState(const Game& game, const MapState& ms) {
  std::stringstream ss;
  boost::archive::text_oarchive oar(ss);
  oar << game;
  oar << ms;
  return ss.str();
}

void FromState(const string& state, Game* game, MapState* ms) {
  std::stringstream ss(state);
  boost::archive::text_iarchive iar(ss);
  iar >> *game;
  iar >> *ms;
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

    cerr << "Send" << endl;
    protocol->Send();
  } else if (protocol->Phase() == GamePhase::kGamePlay) {
    cerr << "GamePlay: state = " << protocol->State() << endl;

    Game game;
    MapState map_state;
    FromState(protocol->State(), &game, &map_state);

    auto sites = game.Map().Sites();

    for (const Move& m : protocol->OtherMoves()) {
      if (map_state.ApplyMove(game.Map(), m) != kOk) {
        cerr << "Illegal move: " << m << endl;
        continue;
      }
    }

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

    protocol->SetPlayerMove(best_move);

    protocol->SetState(MakeState(game, map_state));

    cerr << "Send" << endl;
    protocol->Send();
  } else if (protocol->Phase() == GamePhase::kScoring) {
    cerr << "Scoring" << endl;
    for (auto score : protocol->Scores()) {
      cerr << "PunterID = " << score.PunterID() << ", Score = " << score.Get() << endl;
    }
  }

  return 0;
}
