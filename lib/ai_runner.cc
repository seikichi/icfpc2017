#include <thread>
#include <condition_variable>
#include <unistd.h>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include "protocol.h"
#include "strings.h"
#include "map.h"
using namespace std;

string MakeState(const Game& game, const MapState& ms, int my_rounds) {
  std::stringstream ss;
  boost::archive::text_oarchive oar(ss);
  oar << game;
  oar << ms;
  oar << my_rounds;
  return ss.str();
}

void FromState(const string& state, Game* game, MapState* ms, int* my_rounds) {
  std::stringstream ss(state);
  boost::archive::text_iarchive iar(ss);
  iar >> *game;
  iar >> *ms;
  iar >> *my_rounds;
}

void DoSetup(OfflineClientProtocol* protocol) {
  auto game = protocol->Game();

  MapState initial_map_state(game.Map(), game.PunterNum());
  protocol->SetState(MakeState(game, initial_map_state, 0));
  protocol->Send();
}

Move DecideByFallbackAi(const Game& game, const MapState& map_state) {
  auto& sites = game.Map().Sites();
  int punter_id = game.PunterID();

  bool has_last_resort = false;
  Move last_resort;

  vector<bool> belongs_to_us(sites.size());
  for (auto& edges : game.Map().Graph()) {
    for (auto& edge : edges) {
      if (map_state.Claimer(edge.id) == punter_id) {
        belongs_to_us[edge.src] = true;
        belongs_to_us[edge.dest] = true;
      }
    }
  }

  for (auto& edges : game.Map().Graph()) {
    for (auto& edge : edges) {
      if (map_state.Claimer(edge.id) != -1) {
        continue;
      }
      if (!has_last_resort) {
        auto src = sites[edge.src].original_id;
        auto dest = sites[edge.dest].original_id;
        last_resort = Move::Claim(punter_id, src, dest);
        has_last_resort = true;
      }
      if (!game.Map().Sites()[edge.src].is_mine &&
          !game.Map().Sites()[edge.dest].is_mine &&
          !belongs_to_us[edge.src] &&
          !belongs_to_us[edge.dest]) {
        continue;
      }
      auto src = sites[edge.src].original_id;
      auto dest = sites[edge.dest].original_id;
      return Move::Claim(punter_id, src, dest);
    }
  }

  if (has_last_resort)
    return last_resort;

  return Move::Pass(punter_id);
}

void RunAiWithTimeoutAndDie(
    OfflineClientProtocol* protocol,
    std::function<Move (const Game& game, const MapState& map_state, int my_rounds)> decide,
    int timeout_millis) {

  using namespace chrono;
  steady_clock::time_point deadline = steady_clock::now() + milliseconds(timeout_millis);

  // state から状態を復元
  Game game;
  MapState map_state;
  int my_rounds;
  FromState(protocol->State(), &game, &map_state, &my_rounds);

  // map_state を最新の状態に復元
  for (const Move& m : protocol->OtherMoves()) {
    if (map_state.ApplyMove(game.Map(), m) != kOk) {
      cerr << "Illegal move: " << m << endl;
      continue;
    }
  }

  condition_variable cv;
  mutex mtx;
  Move best_move;
  bool done = false;

  thread main_ai_thread([&best_move, &decide, &game, &map_state, my_rounds, &mtx, &cv, &done]() {
      Move m = decide(game, map_state, my_rounds);

      // 結果をメインスレッドに返す
      {
        unique_lock<mutex> lock(mtx);
        if (!done) {
          best_move = m;
          done = true;
        }
      }
      cv.notify_one();
  });

  thread fallback_thread([&best_move, deadline, &game, &map_state, &mtx, &cv, &done]() {
      // フォールバック用moveを計算する
      Move m = DecideByFallbackAi(game, map_state);

      // デッドラインまで待つ
      while (steady_clock::now() >= deadline) {
        this_thread::sleep_for(milliseconds(50));
      }

      // MainAI がまだ終わってなければ結果を返す
      {
        unique_lock<mutex> lock(mtx);
        if (!done) {
          cerr << "MainAI timeout!! Fallback to weak AI.\n";
          best_move = m;
          done = true;
        }
      }
  });

  // 待つ
  {
    unique_lock<mutex> lock(mtx);
    while (!done) {
      cv.wait(lock);
    }
  }

  // 結果を送信して死ぬ
  protocol->SetPlayerMove(best_move);
  protocol->SetState(MakeState(game, map_state, my_rounds + 1));
  protocol->Send();
  _exit(0);
}

void DoScoring(OfflineClientProtocol* protocol) {
  for (auto score : protocol->Scores()) {
    cerr << "PunterID = " << score.PunterID() << ", Score = " << score.Get() << endl;
  }
}
