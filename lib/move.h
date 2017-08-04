#pragma once
#include <iostream>
#include <cassert>

enum class MoveType {
  kClaim,
  kPass,
};

class Move {
public:
  static Move Claim(int source, int target) {
    return Move(MoveType::kClaim, source, target);
  }

  static Move Pass() {
    return Move(MoveType::kPass, -1, -1);
  }

  MoveType Type() const { return type; }
  int Source() const { return source; }
  int Target() const { return target; }

private:
  Move(MoveType type, int source, int target):
    type(type), source(source), target(target) {}

private:
  const MoveType type;
  const int source;
  const int target;
};

inline std::ostream& operator<<(std::ostream& stream, const Move& move) {
  switch (move.Type()) {
  case MoveType::kClaim:
    stream << "Claim(" << move.Source() << ", " << move.Target() << ")";
    break;
  case MoveType::kPass:
    stream << "Pass";
    break;
  default:
    assert(false);
  }
  return stream;
}
