#include <iostream>
#include <memory>
#include <queue>
#include "protocol.h"
#include "strings.h"
#include "map.h"
#include "scoring.h"

using namespace std;

const int INF = 10000000;

string MakeState(const Game& game, const MapState& ms, int my_rounds) {
  picojson::object o;
  o["game"] = picojson::value(game.SerializeJson());
  o["map_state"] = picojson::value(ms.SerializeJson());
  o["my_rounds"] = picojson::value((double)my_rounds);
  return picojson::value(o).serialize();
}

void FromState(const string& state, Game* game, MapState* ms, int* my_rounds) {
  picojson::object o = StringToJson(state);
  game->Deserialize(o["game"].get<picojson::object>());
  ms->Deserialize(o["map_state"].get<picojson::object>());
  *my_rounds = o["my_rounds"].get<double>();
}

void DoSetup(OfflineClientProtocol* protocol) {
  cerr << "Setup" << endl;
  auto game = protocol->Game();
  cerr << game.SerializeString() << endl;

  MapState initial_map_state(game.Map());
  protocol->SetState(MakeState(game, initial_map_state, 0));

  cerr << "Send" << endl;
  protocol->Send();
}

Move AnyMove(Game& game, const MapState& map_state) {
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

Move Greedy(Game& game, const MapState& map_state) {
  auto& sites = game.Map().Sites();
  int punter_id = game.PunterID();

  vector<bool> connected(game.Map().Sites().size());

  for (auto& edges : game.Map().Graph()) {
    for (auto& edge : edges) {
      if (map_state.Claimer(edge.id) == punter_id) {
        connected[edge.src] = true;
        connected[edge.dest] = true;
      }
    }
  }

  int64_t max_score = -1;
  Move best_move = Move::Pass(punter_id);

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
    // スコアが上がるムーブがないので、適当に一個辺を取る
    return AnyMove(game, map_state);
  }

  return best_move;
}

// mine から各サイトへの距離の配列を返す
vector<int> Bfs(const Map& map, const MapState& map_state, int mine, int punter_id) {
  vector<int> dist(map.Sites().size(), INF);

  queue<int> q;
  q.push(mine);

  while (!q.empty()) {
    int site = q.front(); q.pop();
    for (const Edge& e : map.Graph()[site]) {
      if (map_state.Claimer(e.id) != -1 && map_state.Claimer(e.id) != punter_id)
        continue;
      int new_dist = dist[site] + 1;
      if (new_dist >= dist[e.dest])
        continue;
      dist[e.dest] = new_dist;
      q.push(e.dest);
    }
  }

  return dist;
}

Move Decide(Game& game, const MapState& map_state, int my_rounds) {
  auto& sites = game.Map().Sites();
  int punter_id = game.PunterID();
  const Map& map = game.Map();

  // あと何回行動できるのかを計算する
  int n_rounds = 0;
  for (const auto& es : map.Graph()) {
    n_rounds += (int)es.size();
  }
  assert(n_rounds % 2 == 0);
  n_rounds /= 2;
  int all_my_rounds = n_rounds / game.PunterNum() + (int)(punter_id < n_rounds % game.PunterNum());
  int remaining_rounds = all_my_rounds - my_rounds;

  bool no_our_mines = true;

  // サイトiを自分たちが取っている(繋いでいる)かどうか
  vector<bool> belongs_to_us(sites.size(), false);

  for (auto& es : map.Graph()) {
    for (auto& e : es) {
      if (map_state.Claimer(e.id) == punter_id) {
        belongs_to_us[e.src] = true;
        belongs_to_us[e.dest] = true;
        if (sites[e.src].is_mine || sites[e.dest].is_mine)
          no_our_mines = false;
      }
    }
  }

  // サイトiからまだ取ってない任意の鉱山への距離（他人がclaimしていない辺のみ使用可）
  vector<int> dist_to_any_mine_from_the_site(sites.size(), INF);
  int n_not_our_mines = 0;
  int n_all_mines = 0;

  for (int i = 0; i < (int)sites.size(); ++i) {
    if (!sites[i].is_mine)
      continue;
    ++n_all_mines;
    if (belongs_to_us[i])
      continue;
    ++n_not_our_mines;
    auto& mine = sites[i];
    vector<int> dist_to_the_mine = Bfs(map, map_state, mine.id, punter_id);

    for (int k = 0; k < (int)sites.size(); ++k) {
      dist_to_any_mine_from_the_site[k] = min(
        dist_to_any_mine_from_the_site[k],
        dist_to_the_mine[k]
      );
    }
  }

  cerr << "n_our_mines = " << (n_all_mines - n_not_our_mines) << ", n_all_mines = " << n_all_mines << endl;

  int min_dist = INF;
  Move best_move = Move::Pass(punter_id);

  for (auto& edges : game.Map().Graph()) {
    for (auto& edge : edges) {
      if (map_state.Claimer(edge.id) != -1)
        continue;

      bool is_candidate;
      if (no_our_mines)
        is_candidate = sites[edge.src].is_mine;
      else
        is_candidate = belongs_to_us[edge.src] && !belongs_to_us[edge.dest];
      if (!is_candidate)
        continue;

      int d = dist_to_any_mine_from_the_site[edge.dest];
      if (d > remaining_rounds - 1) {
        // 残りのターンすべてを使ってもたどり着かないなら無視
        // -1 しているのはこの辺を取るのに１ターン消費するから
        continue;
      }
      if (d < min_dist) {
        min_dist = d;
        best_move = Move::Claim(
            punter_id,
            sites[edge.src].original_id,
            sites[edge.dest].original_id);
      }
    }
  }

  if (min_dist == INF) {
    // Greedy にフォールバック
    return Greedy(game, map_state);
  }

  return best_move;
}

void DoGamePlay(OfflineClientProtocol* protocol) {
  cerr << "GamePlay: state = " << protocol->State() << endl;

  Game game;
  MapState map_state(game.Map());
  int my_rounds;
  FromState(protocol->State(), &game, &map_state, &my_rounds);

  for (const Move& m : protocol->OtherMoves()) {
    if (map_state.ApplyMove(game.Map(), m) != kOk) {
      cerr << "Illegal move: " << m << endl;
      continue;
    }
  }

  protocol->SetPlayerMove(Decide(game, map_state, my_rounds));
  protocol->SetState(MakeState(game, map_state, my_rounds + 1));

  protocol->Send();
}

void DoScoring(OfflineClientProtocol* protocol) {
  cerr << "Scoring" << endl;
  for (auto score : protocol->Scores()) {
    cerr << "PunterID = " << score.PunterID() << ", Score = " << score.Get() << endl;
  }
}

int main(int, char**) {
  auto name = "YAGI";
  auto protocol = make_unique<OfflineClientProtocol>();

  protocol->SetPlayerName(name);
  protocol->Receive();

  if (protocol->Phase() == GamePhase::kSetup) {
    DoSetup(protocol.get());
  } else if (protocol->Phase() == GamePhase::kGamePlay) {
    DoGamePlay(protocol.get());
  } else if (protocol->Phase() == GamePhase::kScoring) {
    DoScoring(protocol.get());
  }

  return 0;
}
