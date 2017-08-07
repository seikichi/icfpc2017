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

int dfs(int punter_id, const Graph &g, const MapState& map_state, int start, int from, vector<int> *connected) {
  if (connected->at(from) != -1) { return 0; }
  connected->at(from) = start;
  int ret = 1;
  for (const auto &edge : g[from]) {
    if (map_state.Claimer(edge.id) != punter_id) { continue; }
    ret += dfs(punter_id, g, map_state, start, edge.dest, connected);
  }
  return ret;
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

    vector<int> connected(game.Map().Graph().size(), -1);
    for (auto const &site : game.Map().Sites()) {
      if (!site.is_mine) { continue; }
      dfs(game.PunterID(), game.Map().Graph(), map_state, site.id, site.id, &connected);
    }

    vector<Edge> candidates;
    for (auto edges : game.Map().Graph()) {
      for (auto edge : edges) {
        if (map_state.Claimer(edge.id) != -1) { continue; }
        if (connected[edge.src] == connected[edge.dest]) { continue; }
        candidates.push_back(edge);
      }
    }

    Move move;
    if (candidates.size() != 0) {
      std::uniform_int_distribution<unsigned int> dist(0, 1 << 30);
      std::random_device urandom("/dev/urandom");
      Random rand(dist(urandom));
      auto edge = candidates[rand.next((int)candidates.size())];

      auto sites = game.Map().Sites();
      auto src = sites[edge.src].original_id;
      auto dest = sites[edge.dest].original_id;
      move = Move::Claim(game.PunterID(), src, dest);
    } else {
      move = AnyMove(game, map_state);
    }
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
