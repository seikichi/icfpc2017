#include "strings.h"
using namespace std;

string JoinString(const vector<string>& strings, const string& separator) {
  string message = "";
  for (int i = 0; i < (int)strings.size(); ++i) {
    if (i != 0)
      message += separator;
    message += strings[i];
  }
  return message;
}

