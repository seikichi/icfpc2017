#include <iostream>
#include <unordered_set>
#include <memory>
#include <ctime>
#include "protocol.h"
#include "random.h"

using namespace std;

int main(int, char**) {
  auto name = "YAGI";
  auto protocol = make_unique<OfflineClientProtocol>();

  protocol->SetPlayerName(name);
  protocol->Receive();

  if (protocol->Phase() == GamePhase::kSetup) {
    auto game = protocol->Game();
    protocol->SetState(game.SerializeString());
  } else if (protocol->Phase() == GamePhase::kGamePlay) {
    Game game;
    game.Deserialize(protocol->State());
    auto sites = game.Map().Sites();

    unordered_set<int> lambdas;
    for (auto site : sites) {
      if (site.is_mine) {
        lambdas.insert(site.id);
      }
    }

    vector<Edge> candidates;
    for (auto edges : game.Map().Graph()) {
      for (auto edge : edges) {
        if (lambdas.count(edge.src) || lambdas.count(edge.dest)) {
          candidates.push_back(edge);
        }
      }
    }

    Random rand(time(NULL));
    auto edge = candidates[rand.next(0, candidates.size() - 1)];

    auto src = sites[edge.src].original_id;
    auto dest = sites[edge.dest].original_id;
    auto move = Move::Claim(game.PunterID(), src, dest);

    protocol->SetPlayerMove(move);
    protocol->SetState(game.SerializeString());
  } else if (protocol->Phase() == GamePhase::kScoring) {
    for (auto score : protocol->Scores()) {
      cerr << "PunterID = " << score.PunterID() << ", Score = " << score.Get() << endl;
    }
  }
  protocol->Send();

  return 0;
}
