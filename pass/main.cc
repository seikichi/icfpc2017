#include <iostream>
#include <memory>
#include "protocol.h"

using namespace std;

int main(int, char**) {
  auto name = "YAGI";
  auto protocol = make_unique<OfflineClientProtocol>();

  protocol->SetPlayerName(name);
  protocol->Receive();

  if (protocol->Phase() == GamePhase::kSetup) {
    auto game = protocol->Game();
    protocol->SetState(game.SerializeString());
    protocol->Send();
  } else if (protocol->Phase() == GamePhase::kGamePlay) {
    Game game;
    game.Deserialize(protocol->State());
    protocol->SetPlayerMove(Move::Pass(game.PunterID()));
    protocol->SetState(game.SerializeString());
    protocol->Send();
  } else if (protocol->Phase() == GamePhase::kScoring) {
    for (auto score : protocol->Scores()) {
      cerr << "PunterID = " << score.PunterID() << ", Score = " << score.Get() << endl;
    }
  }

  return 0;
}
