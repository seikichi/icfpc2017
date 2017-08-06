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

int main(int, char**) {
  auto name = "YAGI";
  auto protocol = make_unique<OfflineClientProtocol>();

  protocol->SetPlayerName(name);
  protocol->Receive();

  if (protocol->Phase() == GamePhase::kSetup) {
    auto game = protocol->Game();
    std::stringstream ss;
    boost::archive::text_oarchive oar(ss);
    oar << game;
    protocol->SetState(ss.str());
    // cerr << ss.str() << endl;
    protocol->Send();
  } else if (protocol->Phase() == GamePhase::kGamePlay) {
    std::stringstream ss(protocol->State());
    Game game;
    boost::archive::text_iarchive iar(ss);
    iar >> game;
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

    std::uniform_int_distribution<unsigned int> dist(0, 1 << 30);
    std::random_device urandom("/dev/urandom");
    Random rand(dist(urandom));
    auto edge = candidates[rand.next((int)candidates.size())];

    auto src = sites[edge.src].original_id;
    auto dest = sites[edge.dest].original_id;
    auto move = Move::Claim(game.PunterID(), src, dest);

    protocol->SetPlayerMove(move);
    ss.clear();
    boost::archive::text_oarchive oar(ss);
    oar << game;
    protocol->SetState(ss.str());
    protocol->Send();
  } else if (protocol->Phase() == GamePhase::kScoring) {
    for (auto score : protocol->Scores()) {
      cerr << "PunterID = " << score.PunterID() << ", Score = " << score.Get() << endl;
    }
  }

  return 0;
}
