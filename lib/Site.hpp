
struct Site {
  int id;
  bool is_mine;
  Site() = delete;
  Site(int id) : id(id) {;}
  void setMine(bool flag) { is_mine = flag; }
  ostream &operator<<(ostream &os) const {
    os << "{ ";
    os << "\"id\":" << id << ", ";
    os << "\"is_mine\":" << is_mine << ", ";
    os << " }";
    return os;
  }
};
