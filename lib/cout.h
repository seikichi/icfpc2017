#pragma once
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <iostream>
using namespace std;

template <class T>
ostream &operator<<(ostream &os, const vector<T> &rhs) {
  os << "[ ";
  bool first = true;
  for (const T &v : rhs) {
    if (!first) { os << ", "; }
    first = false;
    os << v;
  }
  os << " ]";
  return os;
}
template <class T>
ostream &operator<<(ostream &os, const deque<T> &rhs) {
  os << "[ ";
  bool first = true;
  for (const T &v : rhs) {
    if (!first) { os << ", "; }
    first = false;
    os << v;
  }
  os << " ]";
  return os;
}
template <class T, class U>
ostream &operator<<(ostream &os, const map<T, U> &rhs) {
  os << "{" << endl;
  bool first = true;
  for (const pair<T, U> &v : rhs) {
    if (!first) { os << "," << endl; }
    first = false;
    os << " " << v.first << " : " << v.second;
  }
  os << endl << "}";
  return os;
}
template <class T>
ostream &operator<<(ostream &os, const set<T> &rhs) {
  os << "{ ";
  bool first = true;
  for (const T &v : rhs) {
    if (!first) { os << ", "; }
    first = false;
    os << v;
  }
  os << " }";
  return os;
}
template <class T, class U>
ostream &operator<<(ostream &os, const pair<T, U> &rhs) {
  os << "( " << rhs.first << ", " << rhs.second << " )";
  return os;
}
