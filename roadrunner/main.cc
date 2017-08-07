#include <iostream>
#include <memory>
#include <queue>
#include <cstdlib>
#include <map>
#include "protocol.h"
#include "strings.h"
#include "map.h"
#include "scoring.h"
#include "ai_runner.h"
#include "cout.h"
#include "union_find.h"

using namespace std;

const int INF = 10000000;

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

void Visit(
    int site,
    int punter_id,
    const Map& map,
    const MapState& map_state,
    /* inout */ vector<bool>* visited) {

  (*visited)[site] = true;

  for (const auto& e : map.Graph()[site]) {
    if ((*visited)[e.dest])
      continue;
    if (map_state.Claimer(e.id) != punter_id && map_state.Claimer(e.id) != -1)
      continue;
    Visit(e.dest, punter_id, map, map_state, visited);
  }
}

// 遠くのサイトを目指す
vector<int> DistToHighScoreSite(const Game& game, const MapState& map_state) {
  auto& sites = game.Map().Sites();
  auto& map = game.Map();
  int punter_id = game.PunterID();

  // 到達可能なサイトを求める
  vector<bool> reachable(sites.size(), false);
  vector<bool> belongs_to_us(sites.size(), false);
  for (auto& edges : game.Map().Graph()) {
    for (auto& edge : edges) {
      if (map_state.Claimer(edge.id) == punter_id) {
        belongs_to_us[edge.src] = true;
        if (!reachable[edge.src]) {
          Visit(edge.src, punter_id, map, map_state, &reachable);
        }
      }
    }
  }

  // まだ取っていないサイトの中で到達可能かつ得点が最大のものを見つけてくる
  int best_site = -1;
  int max_score = -1;
  for (int i = 0; i < (int)sites.size(); ++i) {
    if (!reachable[i])
      continue;
    int64_t score = 0;
    for (int k = 0; k < (int)sites.size(); ++k) {
      if (!sites[k].is_mine || !belongs_to_us[k])
        continue;
      score += map.Dist(i, k) * map.Dist(i, k);
    }
    if (score > max_score) {
      best_site = i;
      max_score = score;
    }
  }

  if (best_site == -1) {
    return vector<int>(sites.size(), INF);
  }

  queue<int> q;
  vector<int> dist(sites.size(), INF);

  q.push(best_site);
  dist[best_site] = 0;

  while (!q.empty()) {
    int site = q.front(); q.pop();
    for (auto& e : map.Graph()[site]) {
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

Move Greedy(const Game& game, const MapState& map_state) {
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

  vector<int> dist_to_hss = DistToHighScoreSite(game, map_state);

  pair<int64_t, int> max_score = make_pair(-1, -INF);
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

      int64_t p_score = ScorePunter(game.PunterID(), game.Map(), tmp_map_state);
      auto score = make_pair(p_score, -dist_to_hss[edge.dest]);
      if (score > max_score) {
        max_score = score;
        best_move = m;
      }
    }
  }

  if (best_move.Type() == MoveType::kPass) {
    // スコアが上がるムーブがないので、適当に一個辺を取る
    return AnyMove(game, map_state);
  }

  return best_move;
}


// // startから連結している点をstartのグループにする
// int DfsForGroupMake(int punter_id, const Graph &g, const MapState& map_state, int start, int from, vector<int> *group) {
//   if (group->at(from) != -1) { return 0; }
//   group->at(from) = start;
//   int ret = 1;
//   for (const auto &edge : g[from]) {
//     if (map_state.Claimer(edge.id) != punter_id) { continue; }
//     ret += dfs(punter_id, g, map_state, start, edge.dest, group);
//   }
//   return ret;
// }


// mine から各サイトへの距離の配列を返す
vector<int> Bfs(const Map& map, const MapState& map_state, int mine, int punter_id, int ban_site_id = -1) {
  vector<int> dist(map.Sites().size(), INF);

  priority_queue<pair<int, int>> q;
  q.push(make_pair(0, mine));
  dist[mine] = 0;

  while (!q.empty()) {
    int site = q.top().second;
    q.pop();
    for (const Edge& e : map.Graph()[site]) {
      if (e.dest == ban_site_id) { continue; }
      if (map_state.Claimer(e.id) != -1 && map_state.Claimer(e.id) != punter_id)
        continue;
      int new_dist = dist[site];
      if (map_state.Claimer(e.id) == -1) { new_dist += 1; }
      if (new_dist >= dist[e.dest])
        continue;
      dist[e.dest] = new_dist;
      q.push(make_pair(new_dist, e.dest));
    }
  }

  return dist;
}

int ChooseBestMine(const Game& game) {
  auto& map = game.Map();
  auto& sites = map.Sites();

  pair<int, int> max_score = make_pair(-1, -1);
  int best_mine = -1;

  for (int i = 0; i < (int)sites.size(); ++i) {
    if (!sites[i].is_mine)
      continue;

    // この鉱山から到達可能な頂点数
    int n_reachable_sites = 0;
    // この鉱山から到達可能な鉱山の数
    int n_reachable_mines = 0;
    // この鉱山から到達可能な頂点への距離の合計
    int sum_dist = 0;

    for (int k = 0; k < (int)sites.size(); ++k) {
      int dist = map.Dist(i, k);
      if (dist >= INF)
        continue;
      ++n_reachable_sites;
      if (sites[k].is_mine)
        ++n_reachable_mines;
      sum_dist += dist;
    }

    pair<int, int> score = make_pair(n_reachable_sites * n_reachable_mines, -sum_dist);
    if (score > max_score) {
      max_score = score;
      best_mine = i;
    }
  }

  return best_mine;
}

vector<vector<int>> FindBridge(const Game &game, MapState temp_map_state, int cnt) {
  const int n = game.Map().Size();
  const Graph graph = game.Map().Graph();
  int best_mine = ChooseBestMine(game);
  vector<vector<int>> ret;
  for (int i = 0; i < cnt; i++) {
    vector<int> prev_dist = Bfs(game.Map(), temp_map_state, best_mine, game.PunterID(), -1);
    vector<int> ban_site_dists_sum(n, 0);
    for (const auto &ban_site : game.Map().Sites()) {
      vector<int> dists = Bfs(game.Map(), temp_map_state, best_mine, game.PunterID(), ban_site.id);
      for (const auto &mine : game.Map().Mines()) {
        if (prev_dist[mine.id] > n) { continue; }
        else if (dists[mine.id] > n) {
          ban_site_dists_sum[ban_site.id] += n;
        } else {
          ban_site_dists_sum[ban_site.id] += dists[mine.id] - prev_dist[mine.id];
        }
      }
    }
    Edge target_edge;
    int max_edge_importance = 0;
    vector<int> edge_importance(game.Map().EdgeNum(), 0);
    for (const auto &edges : graph) {
      for (const auto &edge : edges) {
        if (temp_map_state.Claimer(edge.id) != -1) { continue; }
        edge_importance[edge.id] = min(ban_site_dists_sum[edge.src], ban_site_dists_sum[edge.dest]);
        if (edge_importance[edge.id] > max_edge_importance) {
          target_edge = edge;
          max_edge_importance = edge_importance[edge.id];
        }
      }
    }
    if (max_edge_importance < n) { break; }
    ret.push_back(edge_importance);
    // 自分以外のプレイヤーが取った事にする
    temp_map_state.ApplyMove(game.Map(), Move::Claim((game.PunterID() + 1) % game.PunterNum(), target_edge.src, target_edge.dest));
  }
  return ret;
}

Move DecideByBridge(const Game &game, const MapState &map_state, int my_rounds) {
  MapState temp_map_state = map_state;
  if (my_rounds == 0) {
    // 一手目は初期状態でやる
    temp_map_state = MapState(game.Map(), game.PunterNum());
  }
  vector<vector<int>> edge_importances = FindBridge(game, temp_map_state, 1);
  if (edge_importances.size() == 0) { return Move::Pass(game.PunterID()); }

  Edge target_edge;
  int max_edge_importance = -1;
  for (const auto &edges : game.Map().Graph()) {
    for (const auto &edge : edges) {
      if (map_state.Claimer(edge.id) != -1) { continue; }
      if (edge_importances[0][edge.id] > max_edge_importance) {
        target_edge = edge;
        max_edge_importance = edge_importances[0][edge.id];
      }
    }
  }
  if (max_edge_importance < game.Map().Size()) { return Move::Pass(game.PunterID()); }

  return Move::Claim(game.PunterID(), game.Map().Sites()[target_edge.src].original_id, game.Map().Sites()[target_edge.dest].original_id);
}

Move DecideByRoadRunner(const Game& game, const MapState& map_state, int my_rounds) {
  if (my_rounds < 2 && game.Map().Size() < 500) {
    Move move = DecideByBridge(game, map_state, my_rounds);
    if (move.Type() != MoveType::kPass) { return move; }
  }

  auto& sites = game.Map().Sites();
  int punter_id = game.PunterID();
  const Map& map = game.Map();

  // あと何回行動できるのかを計算する
  int n_rounds = map.EdgeNum();
  int all_my_rounds = n_rounds / game.PunterNum() + (int)(punter_id < n_rounds % game.PunterNum());
  int remaining_rounds = all_my_rounds - my_rounds;



  // サイトiを自分たちが取っている(繋いでいる)かどうか
  UnionFind self_ufind(map.Size());
  for (auto& es : map.Graph()) {
    for (auto& e : es) {
      if (map_state.Claimer(e.id) == punter_id) {
        self_ufind.UnionSet(e.src, e.dest);
      }
    }
  }
  // サイトiを自分たちが取れる(繋いでいる)かどうか
  UnionFind usable_ufind(map.Size());
  for (auto& es : map.Graph()) {
    for (auto& e : es) {
      if (map_state.Claimer(e.id) == punter_id || map_state.Claimer(e.id) == -1) {
        usable_ufind.UnionSet(e.src, e.dest);
      }
    }
  }
  // 各グループで使える辺の数
  vector<int> usable_edge_count(map.Size(), 0);
  for (auto& es : map.Graph()) {
    for (auto& e : es) {
      if (map_state.Claimer(e.id) != -1 || self_ufind.FindSet(e.src, e.dest)) { continue; }
      usable_edge_count[self_ufind.Root(e.src)]++;
    }
  }

  // 初期選択した頂点を伸ばしつづける
  int from_mine = ChooseBestMine(game);

  // mineから出る辺が無くなりそうだったら抑えておく
  // ただし、初期選択した頂点につなげれない場合は諦める
  {
    int min_edge = 100;
    for (const auto &site : map.Mines()) {
      if (self_ufind.FindSet(site.id, from_mine)) { continue; }
      if (usable_edge_count[self_ufind.Root(site.id)] > 5) { continue; }
      if (!usable_ufind.FindSet(site.id, from_mine)) { continue; }
      assert(usable_edge_count[self_ufind.Root(site.id)] != 0);
      if (usable_edge_count[self_ufind.Root(site.id)] < min_edge) {
        fprintf(stderr, "Change from_mine, %d %d\n", site.id, usable_edge_count[self_ufind.Root(site.id)]);
        from_mine = site.id;
        min_edge = usable_edge_count[self_ufind.Root(site.id)];
      }
    }
  }


  // from_mineからまだ取ってない任意の鉱山への距離（他人がclaimしていない辺のみ使用可）
  vector<int> dist_to_any_mine_from_the_site(sites.size(), INF);
  int n_not_our_mines = 0;
  int n_all_mines = 0;

  for (int i = 0; i < (int)sites.size(); ++i) {
    if (!sites[i].is_mine) { continue; }
    ++n_all_mines;
    if (self_ufind.FindSet(i, from_mine)) { continue; }
    ++n_not_our_mines;
    auto& mine = sites[i];
    vector<int> dist_to_the_mine = Bfs(map, map_state, mine.id, punter_id);

    //cerr << "dist_to_the_mine[" << i << "]: " << dist_to_the_mine << endl;

    for (int k = 0; k < (int)sites.size(); ++k) {
      dist_to_any_mine_from_the_site[k] = min(
        dist_to_any_mine_from_the_site[k],
        dist_to_the_mine[k]
      );
    }
  }

  cerr << "n_our_mines = " << (n_all_mines - n_not_our_mines) << ", n_all_mines = " << n_all_mines << endl;
  //cerr << "dist_to_any_mine_from_the_site: " << dist_to_any_mine_from_the_site << endl;

  int min_dist = INF;
  Move best_move = Move::Pass(punter_id);

  for (auto& edges : game.Map().Graph()) {
    for (auto& edge : edges) {
      // 他の人に取られている辺や、とっても意味がない辺は無視
      if (map_state.Claimer(edge.id) != -1 || self_ufind.FindSet(edge.src, edge.dest)) {
        continue;
      }

      // 片方の辺がfrom_mineに繋がっていれば候補
      if (!self_ufind.FindSet(edge.src, from_mine) && !self_ufind.FindSet(edge.dest, from_mine)) {
        continue;
      }

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
    cerr << "[roadrunner] Fallback to Greedy" << endl;
    return Greedy(game, map_state);
  }

  return best_move;
}

int main(int, char**) {
  auto name = "YAGI";
  auto protocol = make_unique<OfflineClientProtocol>();

  protocol->SetPlayerName(name);
  protocol->Receive();

  if (protocol->Phase() == GamePhase::kSetup) {
    DoSetup(protocol.get());
  } else if (protocol->Phase() == GamePhase::kGamePlay) {
    RunAiWithTimeoutAndDie(protocol.get(), DecideByRoadRunner, 750);
  } else if (protocol->Phase() == GamePhase::kScoring) {
    DoScoring(protocol.get());
  }

  return 0;
}
