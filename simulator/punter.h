#pragma once

#include <string>
#include <iostream>
#include "move.h"

class Punter {
public:
  Punter(int id, const std::string& path):
    id(id), path(path) {}

  int Id() const {
    return id;
  }

  const std::string& Path() const {
    return path;
  }

private:
  const int id;
  const std::string path;
};

inline std::ostream& operator<<(std::ostream& stream, const Punter& punter) {
  stream << "Punter(" << punter.Id() << ", \"" << punter.Path() << "\")";
  return stream;
}

struct PunterState {
  int n_timeout = 0;
  bool is_zombie = false;
  std::string state;
  Move prev_move = Move::Pass(-1);
};

inline std::ostream& operator<<(std::ostream& stream, const PunterState& ps) {
  stream << "PunterState("
    << "n_timeout=" << ps.n_timeout << ", "
    << "is_zombie=" << ps.is_zombie << ", "
    << "state=\"" << ps.state << "\")";
  return stream;
}

