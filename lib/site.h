#pragma once
#include <iostream>
using namespace std;
struct Site {
  int id;
  int original_id;
  bool is_mine = false;
  Site() : id(-1), original_id(-1) {;}
  Site(int id, int original_id) : id(id), original_id(original_id) {;}
  void setMine(bool flag) { is_mine = flag; }
  ostream &operator<<(ostream &os) const {
    os << "{ ";
    os << "\"id\":" << id << ", ";
    os << "\"is_mine\":" << is_mine << ", ";
    os << "\"original_id\":" << original_id << ", ";
    os << " }";
    return os;
  }
};
