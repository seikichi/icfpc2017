#include <iostream>
#include <memory>
#include <queue>
#include "protocol.h"
#include "strings.h"
#include "map.h"
#include "scoring.h"

using namespace std;

const int INF = 10000000;

string MakeState(const Game& game, const MapState& ms) {
  picojson::object o;
  o["game"] = picojson::value(game.SerializeJson());
  o["map_state"] = picojson::value(ms.SerializeJson());
  return picojson::value(o).serialize();
}

void FromState(const string& state, Game* game, MapState* ms) {
  picojson::object o = StringToJson(state);
  game->Deserialize(o["game"].get<picojson::object>());
  ms->Deserialize(o["map_state"].get<picojson::object>());
}

void DoSetup(OfflineClientProtocol* protocol) {
  cerr << "Setup" << endl;
  auto game = protocol->Game();
  cerr << game.SerializeString() << endl;

  MapState initial_map_state(game.Map());
  protocol->SetState(MakeState(game, initial_map_state));

  cerr << "Send" << endl;
  protocol->Send();
}

// mine から各サイトへの距離の配列を返す
vector<int> Bfs(const Map& map, MapState& map_state, int mine, int punter_id) {
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

void DoGamePlay(OfflineClientProtocol* protocol) {
  cerr << "GamePlay: state = " << protocol->State() << endl;

  Game game;
  MapState map_state(game.Map());
  FromState(protocol->State(), &game, &map_state);

  auto& sites = game.Map().Sites();

  for (const Move& m : protocol->OtherMoves()) {
    if (map_state.ApplyMove(game.Map(), m) != kOk) {
      cerr << "Illegal move: " << m << endl;
      continue;
    }
  }

  int punter_id = game.PunterID();
  const Map& map = game.Map();

  bool no_our_mines = true;

  // サイトiを自分たちが取っている(繋いでいる)かどうか
  vector<bool> belongs_to_us(sites.size());

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

  for (int i = 0; i < (int)sites.size(); ++i) {
    if (!sites[i].is_mine)
      continue;
    if (belongs_to_us[i])
      continue;
    auto& mine = sites[i];
    vector<int> dist_to_the_mine = Bfs(map, map_state, mine.id, punter_id);

    for (int k = 0; k < (int)sites.size(); ++k) {
      dist_to_any_mine_from_the_site[k] = min(
        dist_to_any_mine_from_the_site[k],
        dist_to_the_mine[k]
      );
    }
  }

  double max_score = -1;
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

      double score = 0;
      for (int i = 0; i < (int)sites.size(); ++i) {
        if (!sites[i].is_mine)
          continue;
        double d = dist_to_any_mine_from_the_site[edge.dest];
        score += 1.0 / d;
      }
      if (score >= max_score) {
        max_score = score;
        best_move = Move::Claim(
            punter_id,
            sites[edge.src].original_id,
            sites[edge.dest].original_id);
      }
    }
  }

  protocol->SetPlayerMove(best_move);
  protocol->SetState(MakeState(game, map_state));

  cerr << "Send" << endl;
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
