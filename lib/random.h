#pragma once
#include <boost/serialization/serialization.hpp>
struct Random {
  unsigned int x;
  unsigned int y;
  unsigned int z;
  unsigned int w; 
  Random() : x(0x34fb2383), y(0x327328fa), z(0xabd4b54a), w(0xa9dba8d1) {;}
  Random(int s) : x(0x34fb2383), y(0x327328fa), z(0xabd4b54a), w(s) {
    for (int i = 0; i < 100; i++) { Xor128(); }
  }
  void Seed(int s) {
    *this = Random(s);
  }
  unsigned int Xor128() {
    unsigned int t;
    t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = (w ^ (w >> 19)) ^ (t ^ (t >> 8)); 
  }
  int next(int r) { return Xor128() % r; }
  int next(int l, int r) { return next(r - l + 1) + l; }
  int64_t next(int64_t r) { return (int64_t)((((unsigned long long)Xor128() << 32) + (unsigned long long)Xor128()) % r); }
  int64_t next(int64_t l, int64_t r) { return next(r - l + 1) + l; }
  double next(double r) { return (double)Xor128() / 0xffffffff * r; }
  double next(double l, double r) { return next(r - l) + l; }

  friend class boost::serialization::access;
  template <class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar & boost::serialization::make_nvp("", x);
      ar & boost::serialization::make_nvp("", y);
      ar & boost::serialization::make_nvp("", z);
      ar & boost::serialization::make_nvp("", w);
    }
};
