#include "scoring.h"
#include <cstdint>

inline int64_t Square(int64_t x) { return x*x; }

int64_t Dfs(
    int site,
    int mine,
    int punter_id,
    const Map& map,
    const MapState& map_state,
    /* inout */ vector<bool>* visited) {

  (*visited)[site] = true;

  int64_t sum = Square(map.Dist(mine, site));
  for (const auto& e : map.Graph()[site]) {
    if ((*visited)[e.dest])
      continue;
    if (map_state.Claimer(e.id) != punter_id)
      continue;
    sum += Dfs(e.dest, mine, punter_id, map, map_state, visited);
  }
  return sum;
}

int64_t ScoreMine(
    int mine,
    int punter_id,
    const Map& map,
    const MapState& map_state) {

  vector<bool> visited(map.Size());
  return Dfs(mine, mine, punter_id, map, map_state, &visited);
}

int64_t ScorePunter(
    const Punter& punter,
    const Map& map,
    const MapState& map_state) {

  int64_t score = 0;
  for (const auto& site : map.Sites()) {
    if (!site.is_mine)
      continue;
    score += ScoreMine(site.id, punter.Id(), map, map_state);
  }
  return score;
}
