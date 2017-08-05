#pragma once

#include "move.h"
#include "map.h"

class BaseAi {
  int punter_id;
  Map map;
protected:
  BaseAi(int id);
  BaseAi(int id, class Map map);
public:
  int PunterId() const { return punter_id; }
  Map Map() const { return map; }
  Move NextMove();
};
