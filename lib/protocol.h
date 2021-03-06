#pragma once

#include <assert.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "future.h"
#include "game.h"
#include "score.h"

using namespace std;

enum class GamePhase {
  kHandshake,
  kFinishHandshake,
  kSetup,
  kGamePlay,
  kScoring,
  kTimeOut,
  kUnknown,
};

class ClientProtocol {
public:
  virtual GamePhase Receive() = 0;
  virtual void Send() = 0;

  GamePhase Phase() { return phase; }
  const ::Game &Game() { return game; }
  const vector<::Move> &OtherMoves() { assert(phase == GamePhase::kGamePlay); return other_moves; }
  const vector<::Score> &Scores() { assert(phase == GamePhase::kScoring); return scores; }
  const string &State() { assert(phase == GamePhase::kGamePlay || phase == GamePhase::kScoring); return prev_state; }

  void SetPlayerName(const string &name) {
    assert(phase == GamePhase::kHandshake);
    player_name = name;
  }
  void SetFutures(const vector<::Future> &futures) { player_futures = futures; }
  void SetPlayerMove(const Move &move) { assert(phase == GamePhase::kGamePlay); player_move = move; }
  void SetState(const string &state) { assert(phase == GamePhase::kSetup || phase == GamePhase::kGamePlay); next_state = state; }

protected:
  virtual std::string ReceiveString() = 0;
  virtual void SendString(const std::string &str) = 0;

  void ReceiveName();
  void SendName();

  GamePhase phase = GamePhase::kHandshake;

  ::Game game;
  vector<::Move> other_moves;
  vector<::Score> scores;
  string prev_state;
  double timeout;

  std::string player_name = "YAGI";
  vector<::Future> player_futures;
  Move player_move;
  string next_state;
};

class OfflineClientProtocol : public ClientProtocol {
public:
  virtual GamePhase Receive() ;
  virtual void Send();
protected:
  virtual std::string ReceiveString();
  std::string ReceiveStringWithState();
  virtual void SendString(const std::string &str);
  void SendStringWithState(const std::string &str, const std::string &state);

  string receive_buffer;
};
