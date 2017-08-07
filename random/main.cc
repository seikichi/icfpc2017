#include <iostream>

#include <unordered_set>
#include <memory>
#include <ctime>
#include <random>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include "protocol.h"
#include "random.h"

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
  game->RecoverFromDeserialize();
}

int main(int, char**) {
  auto name = "YAGI";
  auto protocol = make_unique<OfflineClientProtocol>();

  protocol->SetPlayerName(name);
  protocol->Receive();

  if (protocol->Phase() == GamePhase::kSetup) {
    auto game = protocol->Game();
    MapState initial_map_state(game.Map(), game.PunterNum());
    protocol->SetState(MakeState(game, initial_map_state));
    protocol->Send();
  } else if (protocol->Phase() == GamePhase::kGamePlay) {
    Game game;
    MapState map_state;
    FromState(protocol->State(), &game, &map_state);
    for (const Move& m : protocol->OtherMoves()) {
      if (map_state.ApplyMove(game.Map(), m) != kOk) {
        cerr << "Illegal move: " << m << endl;
        continue;
      }
    }

    vector<Edge> candidates;
    for (auto edges : game.Map().Graph()) {
      for (auto edge : edges) {
        if (map_state.Claimer(edge.id) == -1) {
          candidates.push_back(edge);
        }
      }
    }

    std::uniform_int_distribution<unsigned int> dist(0, 1 << 30);
    std::random_device urandom("/dev/urandom");
    Random rand(dist(urandom));
    auto edge = candidates[rand.next((int)candidates.size())];

    auto sites = game.Map().Sites();
    auto src = sites[edge.src].original_id;
    auto dest = sites[edge.dest].original_id;
    auto move = Move::Claim(game.PunterID(), src, dest);

    protocol->SetPlayerMove(move);
    protocol->SetState(MakeState(game, map_state));
    protocol->Send();
  } else if (protocol->Phase() == GamePhase::kScoring) {
    for (auto score : protocol->Scores()) {
      cerr << "PunterID = " << score.PunterID() << ", Score = " << score.Get() << endl;
    }
  }

  return 0;
}
