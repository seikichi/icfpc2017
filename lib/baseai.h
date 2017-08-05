#pragma once

#include "move.h"

class BaseAi {
  int punter_id;
protected:
  BaseAi(int id);
public:
  int PunterId() const { return punter_id; }
  virtual Move NextMove() = 0;
};
