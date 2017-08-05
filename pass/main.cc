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
    cerr << "Setup" << endl;
    auto game = protocol->Game();
    cerr << game.SerializeString() << endl;
    protocol->SetState(game.SerializeString());
  } else if (protocol->Phase() == GamePhase::kGamePlay) {
    cerr << "GamePlay: state = " << protocol->State() << endl;
    Game game;
    game.Deserialize(protocol->State());
    protocol->SetPlayerMove(Move::Pass(game.PunterID()));
    protocol->SetState(game.SerializeString());
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
